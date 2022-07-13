// gcc -O2 -Wall -o tfits tfits.c -lcfitsio -lm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include <sys/ioctl.h>
#include <fitsio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <libgen.h>

char *version="1.1.2";
struct termios _tbasic, _ttab;

#define VERBOSE 0x01
#define INVERT  0x02
#define ROTATE  0x04
#define SHOWHDR 0x08

#define SHOWEXT 0x10
#define SHOWIMG 0x20
#define SHOWTAB 0x40

#define SPLITEXT 0x80
#define DONTSTOP  0x100
#define OVERWRITE 0x200

#define CONTINUE \
	if(options&DONTSTOP) \
	{\
		if(n<hdu_max) n++;\
		else running=0;\
		continue;\
	}

int options=SHOWIMG|SHOWTAB;
int nHDU=-1;
const char *ascii_chars=" .,;!o%0#";
char ofname[PATH_MAX]="\0";

double (*stretch_fn)(double)=NULL;
float pwr=1.0;

int ow,oh;
size_t outsize;
struct winsize ws;

double **im_outputs=NULL;

void usage()
{
	fprintf(stderr, "usage: tfits [-eHhirSvVx] [-a character set] [-n nHDU] [-o outfile] [-p power] [-s stretch] file.fits[nHDU]\n");
	if(options&VERBOSE)
	{
		fprintf(stderr, "  -a character set : ascii char set (default %s)\n",ascii_chars);
		fprintf(stderr, "  -e               : display extension information\n");
		fprintf(stderr, "  -H               : display header file\n");
		fprintf(stderr, "  -h               : print help message\n");
		fprintf(stderr, "  -i               : invert ascii char set\n");
		fprintf(stderr, "  -n nHDU          : nHDU to print, (synonymous with file[nHDU])\n");
		fprintf(stderr, "  -o outfile.fits  : output filename\n");
		fprintf(stderr, "  -p power         : stretch to exponent p\n");
		fprintf(stderr, "  -r               : rotate image\n");
		fprintf(stderr, "  -S               : split fits extensions\n");
		fprintf(stderr, "  -s stretch       : sin, asin, sinh\n");
		fprintf(stderr, "  -v verbose       : print verbose outputs\n");
		fprintf(stderr, "  -V               : print version\n");
		fprintf(stderr, "  -x               : don't ask to move to next EXTENSION\n");
	}
	exit(1);
}

int usrinput()
{
	int input;
    fd_set set;
    struct timeval tv;
    tv.tv_sec=10;
    tv.tv_usec=0;
    FD_ZERO( &set);
    FD_SET( fileno( stdin ), &set);
    select( fileno( stdin)+1, &set, NULL, NULL, &tv);
    int _ =read( fileno( stdin), &input, 1);
	return input;
}

static inline void nputchar(char c, int n)
{
	for(;n>=0;n--) putchar(c);
}

double *read_pixels(fitsfile *fptr, long naxes[], int *status)
{
	long fpix[2]={1,1};
	double *pixels=(double*)malloc(naxes[0]*naxes[1]*sizeof(double));
	fits_read_pix(fptr, TDOUBLE, fpix, naxes[0]*naxes[1], 0, pixels, NULL, status);
	return pixels;
}

void compress_image(double *dst, double *src, long naxes[])
{

	double _xscale=0.5*(double)ow/naxes[0];
	double _yscale=(double)oh/naxes[1];
	double scale= (_xscale<_yscale) ? _xscale:_yscale;
	memset(dst, 0, sizeof(double)*outsize);

	for(int x=0;x<naxes[0];x++)
	{
		for(int y=0;y<naxes[1];y++)
		{
			int ox=(int)(2*scale*x);
			int oy=(int)(scale*(naxes[1]-y));

			dst[ox + oy*ow]+=src[x+y*naxes[0]];
		}
	}
}

void display_image(double *image)
{
	double maxval=0;
	for(int i=0;i<outsize;i++) maxval=(maxval<image[i])?image[i]:maxval;

	int ai=0, len=strlen(ascii_chars)-1;
	double decimal;
	for(int i=0;i<outsize;i++)
	{
		if(!(i%ow)) printf("|\n|");
		decimal=pow(image[i]/maxval,pwr);
		if(stretch_fn) decimal=stretch_fn(decimal);
		if(options&INVERT) decimal=1-decimal;
		ai=(int)(len*decimal);
		ai=(ai<0) ? 0:ai;
		ai=(ai>len) ? len:ai;
		
		putchar( ascii_chars[ai]);
	}
}

int main(int argc, char *argv[])
{
	int c, status=0;
	fitsfile *fptr=NULL, *ofptr=NULL;

	if(argc==1) 
	{
		options|=VERBOSE;
		usage();
	}
	while((c=getopt(argc, argv, "eHhirSvVxa:n:p:s:"))!=EOF)
	{
		switch(c)
		{
			case 'e': options|=(SHOWEXT|DONTSTOP); 
					  options&= ~(SHOWIMG|SHOWTAB); 
					  break;
			case 'H': options|=(SHOWHDR);
					  options&= ~(SHOWIMG|SHOWTAB); 
					  break;
			case 'h': usage(); break;
			case 'i': options|=INVERT; break;
			case 'r': options|=ROTATE; fprintf(stderr,"Rotate no currently implemented\n");break;
			case 'v': options|=VERBOSE; break;
			case 'V': printf("tfits v%s\n",version); usage(); break;
			case 'x': options|=DONTSTOP; break;

			case 'a': ascii_chars=strdup(optarg); break;
			case 'n': nHDU=atoi(optarg); break;
			case 'o': strncpy(ofname,optarg,PATH_MAX); break;
			case 'p': pwr=atof(optarg); break;
			case 'S': options|=(SPLITEXT|DONTSTOP);
					  options&= ~(SHOWIMG|SHOWTAB); 
					  break;
			case 's':
					  if(!strcmp("log", optarg)) stretch_fn=log;
					  else if(!strcmp("sin", optarg)) stretch_fn=sin;
					  else if(!strcmp("asin", optarg))stretch_fn=asin;
					  else if(!strcmp("sinh", optarg)) stretch_fn=sinh;
					  else if(!strcmp("asinh", optarg)) stretch_fn=asinh;
					  else fprintf(stderr, "Unknown stretch function: %s\n", optarg);
					  break;
		}
	}
	argc-=optind;
	argv+=optind;

	if(argc!=1)
	{
		fprintf(stderr, "No FITS file specified\n");
		usage();
	}

	ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws);
	ow=ws.ws_col-2;
	oh=ws.ws_row-3;
	outsize=oh*ow;

	if(!fits_open_file(&fptr,argv[0], READONLY, &status))
	{
		int n_hdus=0, hdu_min=1, hdu_max=1, type=-1;
		int nkeys=0;
		int running=0;
		char card[FLEN_CARD];

		fits_get_num_hdus(fptr, &n_hdus, &status);
		if(nHDU>0)
		{
			if(nHDU<=n_hdus) hdu_min=hdu_max=nHDU;
			else
			{
				fprintf(stderr, "-n nHDU=%d is out of range for image with %d HDUs.\n",nHDU,n_hdus);
				exit(1);
			}
		}
		else
		{
			hdu_max=n_hdus;
			running=1;
		}

		im_outputs=(double**)malloc((hdu_max-hdu_min+1)* sizeof(double*));
		memset(im_outputs, '\0', (hdu_max-hdu_min+1)* sizeof(double*));

		int n=hdu_min;
		do
		{
			char extname[FLEN_VALUE];
			status=0;

			fits_movabs_hdu(fptr, n, NULL, &status);
			fits_get_hdu_type(fptr, &type, &status);
			fits_get_hdrspace(fptr, &nkeys, NULL, &status);

			if(fits_read_key(fptr, TSTRING, "EXTNAME", extname,NULL,&status))
				if(fits_read_key(fptr, TSTRING, "XTENSION", extname,NULL,&status))
					status=0;

			if(options&SHOWHDR)
			{
				for(int key=1; key<=nkeys; key++)
				{
					if(!fits_read_record(fptr, key, card, &status)) printf("%s\n",card);
				}
				printf("END\n");
			}

			if(type==IMAGE_HDU )
			{
				int ndim=0;
				long naxes[4]={0,0,0,0};
				fits_get_img_dim(fptr, &ndim, &status);
				fits_get_img_size(fptr, 4, naxes, &status);
				if(options&SHOWEXT)
				{
					printf("EXT[%02d] \"%-16s\" IMAGE (",n, extname);
					for(int i=0;i<ndim;i++) printf("%ld,",naxes[i]);
					printf(")");
					printf("\n");

					CONTINUE
				}
				if(options&SPLITEXT)
				{
					//if(!ofname[0])
					{
						char extension[16]="\0";
						strncpy(ofname,argv[0], strlen(argv[0])-5);
						snprintf(extension,16,"-%d.fits",n);
						strcat(ofname,extension);
						printf("split extension -> %s\n",ofname);
						//printf("argv:%s ofname:%s\n",argv[0],ofname);
					}
					fits_create_file(&ofptr,ofname,&status);
					if(ofptr)
					{
						fits_copy_hdu(fptr,ofptr,0,&status);
					}
					if(status)
					{
						fits_report_error(stderr,status);
						status=0;
					}
					fits_close_file(ofptr, &status);
					memset(ofname,'\0',PATH_MAX);

				}
				if((options&SHOWIMG))
				{
					if(ndim==2)
					{
						if(!im_outputs[n-1]) 
						{
							double *pixels=read_pixels(fptr, naxes, &status);
							im_outputs[n-1]=(double*)malloc(outsize * sizeof(double) );
							compress_image(im_outputs[n-1], pixels, naxes);
							free(pixels);
						}
						nputchar('-',ow);
						fprintf(stdout, "\r|%s EXT[%d] %s",argv[0],n, extname);
						display_image(im_outputs[n-1]);
						printf("|\n|");
						nputchar('-',ow);
					}
					else fprintf(stderr, "Only 2D fits images are currently supported, got %dD\n",ndim);
				}

			}
			else if(type==BINARY_TBL || type==ASCII_TBL)
			{
				long nrows=0;
				int	ncols=0;
				fits_get_num_rows(fptr, &nrows, &status);
				fits_get_num_cols(fptr, &ncols, &status);
				if(options&SHOWEXT)
				{
					printf("EXT[%02d] \"%-16s\" TABLE (%d,%ld)",n,extname,ncols, nrows);
					printf("\n");

					CONTINUE
				}
				if(options&SHOWTAB)
				{
					tcgetattr(fileno(stdin), &_tbasic);
					_ttab=_tbasic;
					_ttab.c_lflag &= (~ICANON&~ECHO);
					tcsetattr(fileno(stdin), TCSANOW, &_ttab);
					fprintf(stderr, "BINARY TABLE output not currently supported\n");
					
					tcsetattr(fileno(stdin), TCSANOW, &_tbasic);
				}
			}
			else
			{

			}



			// EXT control
			CONTINUE
			if(hdu_max != hdu_min)
			{
				printf("\r|Move HDU[%d] next(n), previous(p), quit(q):",n);
				char opt[32]="N";
				if(fgets(opt,3,stdin))
				{
					switch(opt[0])
					{
						case 'n': case 'N': case '\n':
							if(n<hdu_max) n++;
							else n=hdu_min;
							break;
						case 'p': case 'P':
							if(n>hdu_min) n--;
							else n=hdu_max;
							break;
						case 'q': case 'Q':
							running=0;
							break;
						default: fprintf(stderr, "Unknown option: %c\n",opt[0]); break;
					}
				}
				fflush(stdout);
			}
			else running=0;
			if(status) fits_report_error(stderr, status);
		}while(running);
		fits_close_file(fptr, &status);
	}
	else fits_report_error(stderr, status);

	return 0;
}
