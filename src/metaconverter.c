// metaconverter.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>        /* for strncpy  */
#include <ctype.h>

/*
#define METADIR ""
#define METAXMLDIR "xmls"
#define METACSVDIR "csvs"
*/

char METADIR[256];
char METAXMLDIR[256];
char METACSVDIR[256];

#define CONVERT_START_DATE 105

#define VERSION_NUMBER  0x3636
/* emaster: 192 bytes
offset   length   desc
0        2        0x36,0x36 version
2        1        x in Fx.DAT
3        8        unknown
11       14       stock symbol ends with byte 0
25       7        unknown
32       16       stock name ends with byte 0
48       16       unknown
64       4        first date YYMMDD
68       4        unknown
72       4        last date  YYMMDD
76       50       unknown
126      4        first date long YYYYMMDD
130      1        unknown
131      5        last dividend paid
135      4        last dividend adjustment rate
139      53       unknown
*/
#define EMASTER_FX                 2
#define EMASTER_SYMBOL             11
#define EMASTER_NAME               32
#define EMASTER_FIRST_DATE         64
#define EMASTER_LAST_DATE          72
#define EMASTER_FIRST_DATE_LONG    126
#define EMASTER_LAST_DIVIDEND      131
#define EMASTER_LAST_DIVIDEND_ADJ  135

/* master: 52 bytes
offset   length   desc
0        1        x in Fx.DAT
1        6        unknown
7        16       stock name ends with byte 0
23       2        unknown
25       4        first date YYMMDD
29       4        last date YYMMDD
33       3        unknown
36       14       stock symbol ends with byte 0
51       3        unknown
*/

#define MASTER_FX                 0
#define MASTER_SYMBOL             36
#define MASTER_NAME               7
#define MASTER_FIRST_DATE         25
#define MASTER_LAST_DATE          29


/*
xmaster: 150 bytes
offset   length   desc
0        1        unknown
1        15       stock symbol ends with byte 0
16       46       stock name ends with byte 0
62       3?       'D' maybe update type
65       2        n in Fn.MWD
67       13       unknown
80       4        end date e.g. 19981125
84       20       unknown
104      4        start date
108      4        start date
112      4        unknown
116      4        end date
120      30       unknown
*/

#define XMASTER_MARK            0
#define XMASTER_SYMBOL          1
#define XMASTER_NAME            16
#define XMASTER_D               62 
#define XMASTER_FN              65
#define XMASTER_END_DATE_1      80
#define XMASTER_START_DATE_1    104
#define XMASTER_START_DATE_2    108
#define XMASTER_END_DATE_2      116

/*
Fx.DAT or Fn.MWD
offset   length   desc
0        4        date YYMMDD
4        4        open
8        4        high
12       4        low
16       4        close
20       4        volume
24       4        open interest
*/


int process_master( void );
int process_emaster( void );
int process_xmaster( void );
int _fmsbintoieee(float *src4, float *dest4);
int process_fdata(const char* symbol, FILE* fdatafile);

int main(int argc, char *argv[])
//int _tmain(int argc, _TCHAR* argv[])
{    
    if(argc < 4)
    {
        printf("**********Usage***********\n");
        printf("Enter 3 arguments\n");
        printf("1: Metastock Folder\n");
        printf("2: Output Xml folder\n");
        printf("3: Output Csv folder\n");
        exit(-1);
    }
    
    strcpy(METADIR,argv[1]);
    strcpy(METAXMLDIR,argv[2]);
    strcpy(METACSVDIR,argv[3]);
    
    printf("%s\n",METADIR);
    printf("%s\n",METAXMLDIR);
    printf("%s\n",METACSVDIR);
    
    process_master();

    //process_emaster();

    process_xmaster();

    return EXIT_SUCCESS;
}

int process_emaster( void )
{
    int end = 0;
    unsigned int num_read;
    unsigned char buffer[192];

    char outstr[100];

    sprintf(outstr, "%s/EMASTER", METADIR);

    FILE* emasterfile = fopen(outstr, "rb");

    if (!emasterfile)
    {
        return -1;
    }

    while (!end)
    {
        if (!feof(emasterfile))
        {
            num_read = fread(buffer, 1, 192, emasterfile);
            /*check the version number*/
            if ((num_read == 192)
                && (buffer[0] == 0x36)
                && (buffer[1] == 0x36)
                )
            {
                unsigned char temp = buffer[EMASTER_FIRST_DATE + 3];
                float out2 = 0.0;
                float out3 = 0.0;

                char datfilename[256];
                char symbol[15];
                FILE* datfile;
                buffer[EMASTER_FIRST_DATE + 3] = (temp << 4) + (temp >> 4);
                temp = buffer[EMASTER_LAST_DATE + 3];
                buffer[EMASTER_LAST_DATE + 3] = (temp << 4) + (temp >> 4);
                _fmsbintoieee((float*)&buffer[EMASTER_FIRST_DATE], &out2);
                _fmsbintoieee((float*)&buffer[EMASTER_LAST_DATE], &out3);
                printf("F%d.DAT: %s, %s, %f, %f\n", buffer[EMASTER_FX],
                    &buffer[EMASTER_SYMBOL],
                    &buffer[EMASTER_NAME],
                    out2, out3);
                sprintf(datfilename, "%s/F%d.DAT", METADIR, buffer[EMASTER_FX]);
                sprintf(symbol, "%s", &buffer[EMASTER_SYMBOL]);
                datfile = fopen(datfilename, "rb");
                if (datfile)
                {
                    process_fdata(symbol, datfile);
                    fclose(datfile);
                }
            }
        }
        else
        {
            end++;
        }
    }
    fclose(emasterfile);
    return 0;
}

int process_master( void )
{
    int end = 0;
    unsigned int num_read;
    unsigned char buffer[53];

    char outstr[100];

    sprintf(outstr, "%s/MASTER", METADIR);

    FILE* masterfile = fopen(outstr, "rb");

    if (!masterfile)
    {
        return -1;
    }

    while (!end)
    {
        if (!feof(masterfile))
        {
            num_read = fread(buffer, 1, 53, masterfile);

            if (num_read == 53)
            {
                char datfilename[256];
                char symbol[256];
                FILE* datfile;
                printf("F%d.DAT: %s, %s\n", buffer[MASTER_FX],
                    &buffer[MASTER_SYMBOL],
                    &buffer[MASTER_NAME]);

                sprintf(datfilename, "%s/F%d.DAT", METADIR, buffer[MASTER_FX]);
                sprintf(symbol, "%s", &buffer[MASTER_SYMBOL]);
                if (symbol[4] == 0x20)
                {
                    symbol[4] = '\0';
                }
                if (symbol[5] == 0x20)
                {
                    symbol[5] = '\0';
                }
                symbol[6] = '\0';
                datfile = fopen(datfilename, "rb");
                if (datfile)
                {
                    process_fdata( symbol, datfile );
                    fclose(datfile);
                }
            }
        }
        else
        {
            end++;
        }
    }
    fclose(masterfile);
    return 0;
}

int process_xmaster( void )
{
    unsigned int num_read;
    unsigned char buffer[150];

    char outstr[100];

    sprintf(outstr, "%s/XMASTER", METADIR);

    FILE* xmasterfile = fopen(outstr, "rb");

    if (!xmasterfile)
    {
        return -1;
    }

    while (!feof(xmasterfile))
    {
        num_read = fread(buffer, 1, 150, xmasterfile);
        /*check the version number*/
        if ((num_read == 150)
            && (buffer[0] == 0x01)
            )
        {
            unsigned int out3, out4;

            char datfilename[256];
            char symbol[15];
            FILE* datfile;

            out3 = (buffer[XMASTER_START_DATE_1] << 24) + \
                    (buffer[XMASTER_START_DATE_1 + 1] << 16) + \
                    (buffer[XMASTER_START_DATE_1 + 2] << 8) + \
                    buffer[XMASTER_START_DATE_1 + 3];

            out4 = (buffer[XMASTER_END_DATE_2] << 24) + \
                    (buffer[XMASTER_END_DATE_2 + 1] << 16) + \
                    (buffer[XMASTER_END_DATE_2 + 2] << 8) + \
                    buffer[XMASTER_END_DATE_2 + 3];

            printf("F%d.MWD: %s, %s, %d, %d\n", (buffer[XMASTER_FN + 1] << 8) + buffer[XMASTER_FN],
                &buffer[XMASTER_SYMBOL],
                &buffer[XMASTER_NAME],
                out3, out4);

            sprintf(datfilename, "%s/F%d.MWD", METADIR, (buffer[XMASTER_FN + 1] << 8) + buffer[XMASTER_FN]);
            sprintf(symbol, "%s", &buffer[XMASTER_SYMBOL]);
            datfile = fopen(datfilename, "rb");

            if (datfile)
            {
                process_fdata( symbol, datfile );
                fclose(datfile);
            }
        }		
    }
    fclose(xmasterfile);
    return 0;
}

int process_fdata( const char* symbol, FILE* fdatafile )
{
    unsigned int num_read;
    unsigned char buffer[24];
    char filename[256];

    int arridx = 0;
    float date, open, close, low, high, amount;

    FILE* xmlfile;
    FILE* csvfile;

    num_read = fread(buffer, 1, 24, fdatafile);
    
    memset(filename, 0, 256);
    sprintf(filename, "%s/%s.xml", METAXMLDIR, symbol);
    xmlfile = fopen(filename, "w+");
    if (!xmlfile)
    {
        return -1;
    }

    memset(filename, 0, 256);
    sprintf(filename, "%s/%s.csv", METACSVDIR, symbol);
    csvfile = fopen(filename, "w+");
    if (!csvfile)
    {
        fclose(xmlfile);
        return -1;
    }
    
    
    fprintf(xmlfile, "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n");
    fprintf(xmlfile, "<stock name=\"%s\">\n", symbol);

    fprintf(csvfile, "Date,Open,Low,High,Close,Volume\n");
    while (!feof(fdatafile))
    {
        num_read = fread(buffer, 1, 24, fdatafile);
        /*check the version number*/
        if (num_read == 24)
        {
            long int am = ((long int)(amount*close));

            _fmsbintoieee((float*)&buffer[0], &date);
            _fmsbintoieee((float*)&buffer[4], &open);
            _fmsbintoieee((float*)&buffer[8], &high);
            _fmsbintoieee((float*)&buffer[12], &low);
            _fmsbintoieee((float*)&buffer[16], &close);
            _fmsbintoieee((float*)&buffer[20], &amount);

            if ((((int)date) / 10000)<CONVERT_START_DATE)
            {
                continue;
            }

            fprintf(xmlfile, "<date id=\"%d\">", (int)date);
            fprintf(xmlfile, "<open>%.4f</open>", open);
            fprintf(xmlfile, "<close>%.4f</close>", close);
            fprintf(xmlfile, "<high>%.4f</high>", high);
            fprintf(xmlfile, "<low>%.4f</low>", low);
            fprintf(xmlfile, "<amount>%ld</amount>", am);
            fprintf(xmlfile, "</date>\n");

            fprintf(csvfile,"%d,%.4f,%.4f,%.4f,%.4f,%.2f\n",(int)date,open,low,high,close,amount);

            arridx++;
        }
    }

    fprintf(xmlfile, "</stock>\n");
    fclose(xmlfile);
    fclose(csvfile);
    return 0;
}

int _fmsbintoieee(float *src4, float *dest4)
{
    unsigned char *msbin = (unsigned char *)src4;
    unsigned char *ieee = (unsigned char *)dest4;
    unsigned char sign = 0x00;
    unsigned char ieee_exp = 0x00;
    int i;

    /* MS Binary Format                         */
    /* byte order =>    m3 | m2 | m1 | exponent */
    /* m1 is most significant byte => sbbb|bbbb */
    /* m3 is the least significant byte         */
    /*      m = mantissa byte                   */
    /*      s = sign bit                        */
    /*      b = bit                             */

    sign = msbin[2] & 0x80;      /* 1000|0000b  */

    /* IEEE Single Precision Float Format       */
    /*    m3        m2        m1     exponent   */
    /* mmmm|mmmm mmmm|mmmm emmm|mmmm seee|eeee  */
    /*          s = sign bit                    */
    /*          e = exponent bit                */
    /*          m = mantissa bit                */

    for (i = 0; i<4; i++) ieee[i] = 0;

    /* any msbin w/ exponent of zero = zero */
    if (msbin[3] == 0) return 0;

    ieee[3] |= sign;

    /* MBF is bias 128 and IEEE is bias 127. ALSO, MBF places   */
    /* the decimal point before the assumed bit, while          */
    /* IEEE places the decimal point after the assumed bit.     */

    ieee_exp = msbin[3] - 2;    /* actually, msbin[3]-1-128+127 */

    /* the first 7 bits of the exponent in ieee[3] */
    ieee[3] |= ieee_exp >> 1;

    /* the one remaining bit in first bin of ieee[2] */
    ieee[2] |= ieee_exp << 7;

    /* 0111|1111b : mask out the msbin sign bit */
    ieee[2] |= msbin[2] & 0x7f;

    ieee[1] = msbin[1];
    ieee[0] = msbin[0];
    return 0;
}

