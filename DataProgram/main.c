#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ifaddrs.h>
#include <netpacket/packet.h>
#include <time.h>

#include "../common/base64.h"
#include "../common/SaveLog.h"

#define VERSION         "2.8.8"
//#define USB_PATH        "/tmp/usb"
//#define USB_PATH        "/tmp/run/mountd/sda1"
//#define USB_PATH        "/mnt"
#define USB_DEV         "/dev/sda1"
#define SDCARD_PATH     "/run/user/1000/sdcard"
#define DEF_PATH        "/run/user/1000"
#define XML_PATH        "/run/user/1000/XML"
#define LOG_XML_PATH    XML_PATH"/LOG"
#define ERRLOG_XML_PATH XML_PATH"/ERRLOG"
#define BMS_PATH        DEF_PATH"/BMS"
#define SYSLOG_PATH     DEF_PATH"/SYSLOG"
#define TODOLIST_PATH   "/run/user/1000/TODOList"
#define WL_CHANGED_PATH "/run/user/1000/WL_Changed"
#define CURL_FILE       "/run/user/1000/Curlfile"
#define TIMEOUT         "10"
#define CURL_CMD        "curl -H 'Content-Type: text/xml;charset=UTF-8;SOAPAction:\"\"' -k https://52.9.235.220:8443/SmsWebService1.asmx?WSDL -d @"CURL_FILE" --max-time "TIMEOUT
#define UPDATE_MAX      200

#define MODEL_LIST_PATH "/home/linaro/bin/ModelList"

char SOAP_HEAD[] =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<soap:Envelope xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">\n\
\t<soap:Body>\n";

char SOAP_TAIL[] = "\t</soap:Body>\n</soap:Envelope>";

char SWVER[16] = {0};
char MAC[18] = {0};
char SMS_SERVER[128] = {0};
int sms_port = 0;
char g_USB_PATH[128] = {0};
char g_ROOT_PATH[128] = {0};
char g_XML_PATH[128] = {0};
char g_LOG_PATH[128] = {0};
char g_ERRLOG_PATH[128] = {0};
char g_ENV_PATH[128] = {0};
char g_BMS_PATH[128] = {0};
char g_SYSLOG_PATH[128] = {0};
int data_interval = 0; // sec
int update_interval = 60; // sec
int shelf_life = 0; // day
int save_time = 3600*24*30;
char g_CURL_CMD[256] = {0};
int check_milist = 1;
int clean_bms_flag = 1;

struct tm st_log_time = {0};
struct tm st_errlog_time = {0};
struct tm st_list_time = {0};

void getMAC(char *MAC)
{
    /*struct ifaddrs *ifaddr = NULL;
    struct ifaddrs *ifa = NULL;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
    }
    else
    {
        for ( ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if ( (ifa->ifa_addr) && (ifa->ifa_addr->sa_family == AF_PACKET) )
            {
                struct sockaddr_ll *s = (struct sockaddr_ll*)ifa->ifa_addr;
                if (s->sll_addr[0]|s->sll_addr[1]|s->sll_addr[2]|s->sll_addr[3]|s->sll_addr[4]|s->sll_addr[5]) {
                    sprintf(MAC, "%02X:%02X:%02X:%02X:%02X:%02X",
                    s->sll_addr[0],s->sll_addr[1],s->sll_addr[2],s->sll_addr[3],s->sll_addr[4],s->sll_addr[5]);
                    MAC[17] = 0;
                    break;
                }
            }
        }
        freeifaddrs(ifaddr);
    }
    printf("getMAC = %s\n", MAC);*/

    FILE *fd = NULL;

    // get MAC address
    fd = popen("cat /sys/class/net/eth0/address", "r");
    if ( fd == NULL ) {
        printf("popen fail!\n");
        return;
    }
    fgets(MAC, 18, fd);
    pclose(fd);

    printf("MAC = %s\n", MAC);

    return;
}

void init()
{
    getMAC(MAC);
    //getConfig();
    //setCMD();
    //setPath();

    return;
}

void getConfig()
{
    char buf[32] = {0};
    FILE *fd = NULL;
    int tmp_interval = 0;

    // get sms server
    fd = popen("/home/linaro/bin/parameter.sh get sms_server", "r");
    if ( fd == NULL ) {
        printf("popen fail!\n");
        return;
    }
    fgets(SMS_SERVER, 128, fd);
    pclose(fd);
    if ( strlen(SMS_SERVER) )
        SMS_SERVER[strlen(SMS_SERVER)-1] = 0; // clean \n
    printf("SMS_SERVER = %s\n", SMS_SERVER);

    // get sms server port
    fd = popen("/home/linaro/bin/parameter.sh get sms_port", "r");
    if ( fd == NULL ) {
        printf("popen fail!\n");
        return;
    }
    fgets(buf, 32, fd);
    pclose(fd);
    sscanf(buf, "%d", &sms_port);
    printf("SMS PORT = %d\n", sms_port);

    // get data interval
/*    fd = popen("/home/linaro/bin/parameter.sh get sample_time", "r");
    if ( fd == NULL ) {
        printf("popen fail!\n");
        return;
    }
    fgets(buf, 32, fd);
    pclose(fd);
    sscanf(buf, "%d", &tmp_interval);
    if ( data_interval == tmp_interval ) {
        printf("same data interval %d\n", data_interval);
        return;
    }
    data_interval = tmp_interval;
    printf("set data interval %d\n", data_interval);
*/
    // get upload interval
    fd = popen("/home/linaro/bin/parameter.sh get upload_time", "r");
    if ( fd == NULL ) {
        printf("popen fail!\n");
        return;
    }
    fgets(buf, 32, fd);
    pclose(fd);
    sscanf(buf, "%d", &tmp_interval);
    if ( update_interval == tmp_interval ) {
        printf("same update interval %d\n", update_interval);
    } else {
        update_interval = tmp_interval;
        printf("set update interval %d\n", update_interval);
    }

    // get shelf life
    fd = popen("/home/linaro/bin/parameter.sh get shelf_life", "r");
    if ( fd == NULL ) {
        printf("popen fail!\n");
        return;
    }
    fgets(buf, 32, fd);
    pclose(fd);
    sscanf(buf, "%d", &tmp_interval);
    if ( shelf_life == tmp_interval ) {
        printf("same shelf life %d\n", shelf_life);
    } else {
        shelf_life = tmp_interval;
        printf("set shelf life %d\n", shelf_life);
    }

    // get swupdate version
    fd = popen("/home/linaro/bin/parameter.sh get dsuver", "r");
    if ( fd == NULL ) {
        printf("popen fail!\n");
        return;
    }
    fgets(SWVER, 16, fd);
    pclose(fd);
    if ( strlen(SWVER) )
        SWVER[strlen(SWVER)-1] = 0; // clean \n
    printf("SWVER = %s\n", SWVER);

    return;
}

void setCMD()
{
    if ( strlen(SMS_SERVER) )
        sprintf(g_CURL_CMD, "curl -H 'Content-Type: text/xml;charset=UTF-8;SOAPAction:\"\"' -k %s:%d/SmsWebService1.asmx?WSDL -d @%s --max-time %s", SMS_SERVER, sms_port, CURL_FILE, TIMEOUT);
    else
        sprintf(g_CURL_CMD, "curl -H 'Content-Type: text/xml;charset=UTF-8;SOAPAction:\"\"' http://60.251.36.232/SmsWebService1.asmx?WSDL -d @%s --max-time %s", CURL_FILE, TIMEOUT);

    return;
}

void setPath()
{
    struct stat st;
	FILE *pFile = NULL;

    if ( stat(USB_DEV, &st) == 0 ) {
	pFile = popen("df | grep /dev/sda1 | awk '{print $6}'", "r");
	if ( pFile == NULL ) {
		printf("popen fail!\n");
		return;
	}
	fgets(g_USB_PATH, 128, pFile);
	pclose(pFile);
	g_USB_PATH[strlen(g_USB_PATH)-1] = 0; // clean \n
	//printf("USB path = %s\n", g_USB_PATH);
	strcpy(g_ROOT_PATH, g_USB_PATH);
    } else if ( stat(SDCARD_PATH, &st) == 0 )
        strcpy(g_ROOT_PATH, SDCARD_PATH);
    else
        strcpy(g_ROOT_PATH, DEF_PATH);

    // set xml path
    sprintf(g_XML_PATH, "%s/XML", g_ROOT_PATH);

    // set log path
    sprintf(g_LOG_PATH, "%s/LOG", g_XML_PATH);

    // set errlog path
    sprintf(g_ERRLOG_PATH, "%s/ERRLOG", g_XML_PATH);

    // set env path
    sprintf(g_ENV_PATH, "%s/ENV", g_XML_PATH);

    // set bms path
    sprintf(g_BMS_PATH, "%s/BMS", g_ROOT_PATH);

    // set syslog path
    sprintf(g_SYSLOG_PATH, "%s/SYSLOG", g_ROOT_PATH);

    return;
}

int CheckTime(struct tm target, struct tm file, int save_sec)
{
    time_t ttarget = 0;
    time_t tfile = 0;
    long int result = 0;

    ttarget = mktime(&target);
    tfile = mktime(&file);
    result = tfile - ttarget + save_sec;

    //printf("ttarget = %ld\n", ttarget);
    //printf("tfile   = %ld\n", tfile);
    //printf("result  = %ld\n", result);

    if ( result > 0 )
        return 1;
    else
        return 0;
}

int PostMIList()
{
    char FILENAME[64] = {0};
    char buf[512] = {0}; // PostMIList result date length about = 372
    FILE *pMIList_fd = NULL;
    FILE *pcurlfile_fd = NULL;
    FILE *presult_fd = NULL;
    struct tm st_file_time = {0};
    char *index = NULL;
    int ret = 0;
    time_t  current_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("======================== PostMIList start ========================\n");
    // get MIlist list (always one new file)
    system("cd /run/user/1000; ls MIList_* > /run/user/1000/MIList");

    pMIList_fd = fopen("/run/user/1000/MIList", "rb");
    if ( pMIList_fd == NULL ) {
        printf("#### Open /run/user/1000/MIList Fail ####\n");
        return 1;
    }
    // get file name
    memset(buf, 0, 512);
    fgets(buf, 64, pMIList_fd);
    fclose(pMIList_fd);
    if ( strlen(buf) )
        buf[strlen(buf)-1] = 0; // set '\n' to 0
    else {
        printf("Empty file! Plese check MIList exist!\n");
        return 2;
    }

    sprintf(FILENAME, "/run/user/1000/%s", buf);
    //printf("FILENAME = %s\n", FILENAME);

    // get file time
    sscanf(buf, "MIList_%04d%02d%02d_%02d%02d%02d",
        &st_file_time.tm_year, &st_file_time.tm_mon, &st_file_time.tm_mday, &st_file_time.tm_hour, &st_file_time.tm_min, &st_file_time.tm_sec);
    st_file_time.tm_year -= 1900;
    st_file_time.tm_mon -= 1;
    //printf("File time : %04d/%02d/%02d %02d:%02d:%02d\n",
    //    st_file_time.tm_year+1900, st_file_time.tm_mon+1, st_file_time.tm_mday, st_file_time.tm_hour, st_file_time.tm_min, st_file_time.tm_sec);

    if ( check_milist )
        printf("CheckWhiteListUploaded() not succes, run PostMIList() again\n");
    else {
        // check time,
        if ( CheckTime(st_list_time, st_file_time, 0) ) {
            printf("Find new MIList\n");
            // set new time to st_list_time
            //st_list_time = st_file_time;
            //printf("List time : %04d/%02d/%02d %02d:%02d:%02d\n",
            //    st_list_time.tm_year+1900, st_list_time.tm_mon+1, st_list_time.tm_mday, st_list_time.tm_hour, st_list_time.tm_min, st_list_time.tm_sec);
        } else {
            printf("Old MIList file %s\n", FILENAME);
            //printf("Not to do update\n");
            return 3;
        }
    }

    // set curl file
    pcurlfile_fd = fopen(CURL_FILE, "wb");
    if ( pcurlfile_fd == NULL ) {
        printf("#### Open %s Fail ####\n", CURL_FILE);
        return 4;
    }
    fputs(SOAP_HEAD, pcurlfile_fd);
    sprintf(buf, "\t\t<PostMIList xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, pcurlfile_fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, pcurlfile_fd);
    sprintf(buf, "\t\t\t<MIList>");
    fputs(buf, pcurlfile_fd);
    base64_encode(FILENAME, pcurlfile_fd);
    sprintf(buf, "</MIList>\n");
    fputs(buf, pcurlfile_fd);
    sprintf(buf, "\t\t</PostMIList>\n");
    fputs(buf, pcurlfile_fd);
    fputs(SOAP_TAIL, pcurlfile_fd);
    fclose(pcurlfile_fd);

    // run curl soap to web server
    sprintf(buf, "%s > /run/user/1000/PostMIList", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/PostMIList", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### PostMIList() /run/user/1000/PostMIList empty ####\n");
            SaveLog("DataProgram PostMIList() : /run/user/1000/PostMIList empty", st_time);
            return 5;
        }
    // read data
    presult_fd = fopen("/run/user/1000/PostMIList", "rb");
    if ( presult_fd == NULL ) {
        printf("#### Open /run/user/1000/PostMIList Fail ####\n");
        return 6;
    }
    memset(buf, 0, 512);
    fread(buf, 1, 512, presult_fd);
    fclose(presult_fd);
    //printf("/run/user/1000/PostMIList : \n%s\n", buf);

    index = strstr(buf, "<PostMIListResult>");
    if ( index == NULL ) {
        printf("#### PostMIList() <PostMIListResult> not found ####\n");
        SaveLog("DataProgram PostMIList() : <PostMIListResult> not found", st_time);
        return 7;
    }
    sscanf(index, "<PostMIListResult>%02d</PostMIListResult>", &ret);
    //printf("ret = %02d\n", ret);

    if ( ret == 0 ) {
        printf("File %s update OK\n", FILENAME);
        SaveLog("DataProgram PostMIList() : update OK", st_time);
        memcpy(&st_list_time, &st_file_time, sizeof(st_file_time));
        printf("========================= PostMIList end =========================\n");
        return 0;
    } else {
        printf("File %s update Fail\n", FILENAME);
        SaveLog("DataProgram PostMIList() : update Fail", st_time);
        printf("========================= PostMIList end =========================\n");
        return 8;
    }
}

int CheckWhiteListUploaded()
{
    // CheckWhiteListUploaded result date length about = 403
    char buf[512] = {0};
    char *index = NULL;
    FILE *fd = NULL;
    time_t current_time;
    struct tm *st_time;
    int ret = 0;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("====================== CheckWhiteListUploaded start ======================\n");
    // set CheckWhiteListUploaded xml file
    fd = fopen(CURL_FILE, "wb");
    if ( fd == NULL ) {
        printf("#### CheckWhiteListUploaded() open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    memset(buf, 0, 512);
    fputs(SOAP_HEAD, fd);
    sprintf(buf, "\t\t<CheckWhiteListUploaded xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, fd);
    sprintf(buf, "\t\t</CheckWhiteListUploaded>\n");
    fputs(buf, fd);
    fputs(SOAP_TAIL, fd);
    fclose(fd);

    // run curl soap command, save result to /run/user/1000/CheckWhiteListUploaded
    sprintf(buf, "%s > /run/user/1000/CheckWhiteListUploaded", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/CheckWhiteListUploaded", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### CheckWhiteListUploaded() /run/user/1000/CheckWhiteListUploaded empty ####\n");
            SaveLog("DataProgram CheckWhiteListUploaded() : /run/user/1000/CheckWhiteListUploaded empty", st_time);
            return 2;
        }
    // read result
    fd = fopen("/run/user/1000/CheckWhiteListUploaded", "rb");
    if ( fd == NULL ) {
        printf("#### CheckWhiteListUploaded() open /run/user/1000/CheckWhiteListUploaded Fail ####\n");
        return 3;
    }
    memset(buf, 0, 512);
    fread(buf, 1, 512, fd);
    fclose(fd);
    //printf("/run/user/1000/CheckWhiteListUploaded : \n%s\n", buf);

    // check result
    index = strstr(buf, "<CheckWhiteListUploadedResult>");
    if ( index == NULL ) {
        printf("#### CheckWhiteListUploaded() <CheckWhiteListUploadedResult> not found ####\n");
        SaveLog("DataProgram CheckWhiteListUploaded() : <CheckWhiteListUploadedResult> not found", st_time);
        index = strstr(buf, "<CheckWhiteListUploadedResult />");
        if ( index == NULL )
            printf("#### CheckWhiteListUploaded() <CheckWhiteListUploadedResult /> not found ####\n");
        else {
            printf("<CheckWhiteListUploadedResult /> find, result data not exist!\n");
            SaveLog("DataProgram CheckWhiteListUploaded() : result data not exist", st_time);
        }

        return 4;
    }
    sscanf(index, "<CheckWhiteListUploadedResult>%02d</CheckWhiteListUploadedResult>", &ret);
    printf("ret = %02d\n", ret);
    if ( ret == 0 ) {
        check_milist = 0;
        printf("CheckWhiteListUploaded update OK\n");
        SaveLog("DataProgram CheckWhiteListUploaded() : update OK", st_time);
        printf("======================= CheckWhiteListUploaded end =======================\n");
        return 0;
    } else {
        printf("CheckWhiteListUploaded update Fail\n");
        SaveLog("DataProgram CheckWhiteListUploaded() : update Fail", st_time);
        printf("======================= CheckWhiteListUploaded end =======================\n");
        return 5;
    }

}

int Qrylogdata()
{
    // Qrylogdata result date length about = 371
    char buf[512] = {0};
    char *index = NULL;
    FILE *fd = NULL;
    time_t current_time;
    time_t data_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("======================== Qrylogdata start ========================\n");
    // set Qrylogdata xml file
    fd = fopen(CURL_FILE, "wb");
    if ( fd == NULL ) {
        printf("#### Qrylogdata() open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    fputs(SOAP_HEAD, fd);
    sprintf(buf, "\t\t<Qrylogdata xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, fd);
    sprintf(buf, "\t\t</Qrylogdata>\n");
    fputs(buf, fd);
    fputs(SOAP_TAIL, fd);
    fclose(fd);

    // run curl soap command, save result to /run/user/1000/Qrylogdata
    sprintf(buf, "%s > /run/user/1000/Qrylogdata", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/Qrylogdata", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### Qrylogdata() /run/user/1000/Qrylogdata empty ####\n");
            SaveLog("DataProgram Qrylogdata() : /run/user/1000/Qrylogdata empty", st_time);
            return 2;
        }
    // read result
    fd = fopen("/run/user/1000/Qrylogdata", "rb");
    if ( fd == NULL ) {
        printf("#### Qrylogdata() open /run/user/1000/Qrylogdata Fail ####\n");
        return 3;
    }
    memset(buf, 0, 512);
    fread(buf, 1, 512, fd);
    fclose(fd);
    //printf("/run/user/1000/Qrylogdata : \n%s\n", buf);

    // set time
    index = strstr(buf, "<QrylogdataResult>");
    if ( index == NULL ) {
        printf("#### Qrylogdata() <QrylogdataResult> not found ####\n");
        SaveLog("DataProgram Qrylogdata() : <QrylogdataResult> not found", st_time);
        index = strstr(buf, "<QrylogdataResult />");
        if ( index == NULL ) {
            printf("#### Qrylogdata() <QrylogdataResult /> not found ####\n");
            st_log_time.tm_year = 0;
            st_log_time.tm_mon = 0;
            st_log_time.tm_mday = 0;
            st_log_time.tm_hour = 0;
            st_log_time.tm_min = 0;
            st_log_time.tm_sec = 0;
            printf("========================= Qrylogdata end =========================\n");
            return 4;
        }
        else {
            printf("<QrylogdataResult /> find, last time data not exist!\n");
            SaveLog("DataProgram Qrylogdata() : last time data not exist", st_time);
            //data_time = current_time - (update_interval*60);
            data_time = current_time - (60*60);
            st_log_time = *localtime(&data_time);
            printf("Set Qrylogdata time : %04d/%02d/%02d %02d:%02d:%02d\n",
            st_log_time.tm_year+1900, st_log_time.tm_mon+1, st_log_time.tm_mday, st_log_time.tm_hour, st_log_time.tm_min, st_log_time.tm_sec);
            printf("========================= Qrylogdata end =========================\n");
            return 0;
        }
    }

    sscanf(index, "<QrylogdataResult>%04d/%02d/%02d %02d:%02d:%02d</QrylogdataResult>",
           &st_log_time.tm_year, &st_log_time.tm_mon, &st_log_time.tm_mday, &st_log_time.tm_hour, &st_log_time.tm_min, &st_log_time.tm_sec);
    st_log_time.tm_year -= 1900;
    st_log_time.tm_mon -= 1;
    printf("Get Qrylogdata time : %04d/%02d/%02d %02d:%02d:%02d\n",
            st_log_time.tm_year+1900, st_log_time.tm_mon+1, st_log_time.tm_mday, st_log_time.tm_hour, st_log_time.tm_min, st_log_time.tm_sec);

    printf("========================= Qrylogdata end =========================\n");
    sprintf(buf, "DataProgram Qrylogdata() : %04d/%02d/%02d %02d:%02d:%02d",
            st_log_time.tm_year+1900, st_log_time.tm_mon+1, st_log_time.tm_mday, st_log_time.tm_hour, st_log_time.tm_min, st_log_time.tm_sec);
    SaveLog(buf, st_time);
    return 0;
}

int Rcvlog()
{
    char FILENAME[64] = {0};
    char buf[512] = {0}; // Rcvlog result date length about = 338
    FILE *pdate_fd = NULL;
    FILE *ptime_fd = NULL;
    FILE *pcurlfile_fd = NULL;
    FILE *presult_fd = NULL;
    struct tm st_file_time = {0};
    char *index = NULL;
    int ret = 0;
    int logdate = 0;
    int qrylogdate = 0;
    int cnt = 0;
    struct stat st;
    int clean = 0, upok = 0;
    time_t current_time;
    struct tm *st_time;
    int i = 0, j = 0;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("======================== Rcvlog start ========================\n");
    if ( strstr(g_LOG_PATH, g_USB_PATH) )
        j = 2; // storage & /run/user/1000
    else
        j = 1; // only /run/user/1000

    for ( i = 0; i < j; i++ ) {
        if ( i == 1 ) // change path from storage to /run/user/1000
            strcpy(g_LOG_PATH, LOG_XML_PATH);

        // get Log file date list
        sprintf(buf, "cd %s; ls > /run/user/1000/LogDate", g_LOG_PATH);
        //printf("buf cmd = %s\n", buf);
        system(buf);

        pdate_fd = fopen("/run/user/1000/LogDate", "rb");
        if ( pdate_fd == NULL ) {
            printf("#### Open /run/user/1000/LogDate Fail ####\n");
            return 1;
        }

        qrylogdate = (st_log_time.tm_year+1900)*10000 + (st_log_time.tm_mon+1)*100 + st_log_time.tm_mday;
        //printf("QryLogDate = %d\n", qrylogdate);

        // get file date
        memset(buf, 0, 512);
        while ( fgets(buf, 64, pdate_fd) != NULL ) {
            if ( strlen(buf) == 0 )
                break;
            // set '\n' to 0
            buf[strlen(buf)-1] = 0;
            sscanf(buf, "%d", &logdate);
            //printf("Get LogDate = %d\n", logdate);
            if ( logdate < qrylogdate ) {
                //printf("Old date, continue\n");
                if ( strstr(g_LOG_PATH, LOG_XML_PATH) != NULL ) {
                    sprintf(buf, "rm -rf %s/%d", g_LOG_PATH, logdate);
                    //printf("cmd = %s\n", buf);
                    system(buf);
                }
                continue;
            }
            sprintf(buf, "cd %s/%d; ls > /run/user/1000/LogTime", g_LOG_PATH, logdate);
            //printf("buf cmd = %s\n", buf);
            system(buf);

            ptime_fd = fopen("/run/user/1000/LogTime", "rb");
            if ( ptime_fd == NULL ) {
                printf("#### Open /run/user/1000/LogTime Fail ####\n");
                return 2;
            }
            // get file time
            memset(buf, 0, 512);
            while ( fgets(buf, 64, ptime_fd) != NULL ) {
                if ( strlen(buf) == 0 )
                    break;
                // set '\n' to 0
                buf[strlen(buf)-1] = 0;

                // remove previous data if save as host (ex. /run/user/1000)
                if ( strstr(FILENAME, LOG_XML_PATH) )
                    remove(FILENAME);
                // remove empty file
                if ( stat(FILENAME, &st) == 0 )
                    if ( st.st_size <= 20 ) {
                            remove(FILENAME);
                            printf("remove empty file!\n");
                    }
                // clean file
                if ( clean )
                    remove(FILENAME);

                // get next file name in list
                sprintf(FILENAME, "%s/%d/%s", g_LOG_PATH, logdate, buf);
                //printf("FILENAME = %s\n", FILENAME);
                // get file time
                st_file_time.tm_year = logdate/10000 - 1900;
                st_file_time.tm_mon = (logdate%10000)/100 - 1;
                st_file_time.tm_mday = logdate%100;
                sscanf(buf, "%02d%02d%02d", &st_file_time.tm_hour, &st_file_time.tm_min, &st_file_time.tm_sec);
                //st_file_time.tm_sec = 0;
                //printf("File time : %04d/%02d/%02d %02d:%02d\n",
                //    st_file_time.tm_year+1900, st_file_time.tm_mon+1, st_file_time.tm_mday, st_file_time.tm_hour, st_file_time.tm_min);
                // check time,
                if ( !CheckTime(st_log_time, st_file_time, 0) ) {
                    //printf("Old file %s\n", FILENAME);
                    //printf("Not to do update\n");
                    //if ( !CheckTime(st_log_time, st_file_time, save_time) ) {
                    //    printf("File too old, remove file %s\n", FILENAME);
                    //    remove(FILENAME);
                    //}
                    continue;
                }
                // check file
                if ( stat(FILENAME, &st) == 0 )
                    if ( st.st_size <= 20 ) {
                        printf("empty file!\n");
                        clean = 1;
                        continue;
                    }

                // set curl file
                pcurlfile_fd = fopen(CURL_FILE, "wb");
                if ( pcurlfile_fd == NULL ) {
                    printf("#### Open %s Fail ####\n", CURL_FILE);
                    continue;
                }
                fputs(SOAP_HEAD, pcurlfile_fd);
                sprintf(buf, "\t\t<Rcvlog xmlns=\"http://tempuri.org/\">\n");
                fputs(buf, pcurlfile_fd);
                sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
                fputs(buf, pcurlfile_fd);
                sprintf(buf, "\t\t\t<devicefile>");
                fputs(buf, pcurlfile_fd);
                base64_encode(FILENAME, pcurlfile_fd);
                sprintf(buf, "</devicefile>\n");
                fputs(buf, pcurlfile_fd);
                sprintf(buf, "\t\t</Rcvlog>\n");
                fputs(buf, pcurlfile_fd);
                fputs(SOAP_TAIL, pcurlfile_fd);
                fclose(pcurlfile_fd);

                // run curl soap to web server
                sprintf(buf, "%s > /run/user/1000/Rcvlog", g_CURL_CMD);
                system(buf);

                // check responds
                if ( stat("/run/user/1000/Rcvlog", &st) == 0 )
                    if ( st.st_size == 0 ) {
                        printf("#### Rcvlog() /run/user/1000/Rcvlog empty ####\n");
                        SaveLog("DataProgram Rcvlog() : /run/user/1000/Rcvlog empty", st_time);
                        //continue;
                        fclose(ptime_fd);
                        fclose(pdate_fd);
                        return 3;
                    }
                // read data
                presult_fd = fopen("/run/user/1000/Rcvlog", "rb");
                if ( presult_fd == NULL ) {
                    printf("#### Open /run/user/1000/Rcvlog Fail ####\n");
                    continue;
                }
                memset(buf, 0, 512);
                fread(buf, 1, 512, presult_fd);
                fclose(presult_fd);
                //printf("/run/user/1000/Rcvlog : \n%s\n", buf);

                index = strstr(buf, "<RcvlogResult>");
                if ( index == NULL ) {
                    printf("#### Rcvlog() <RcvlogResult> not found ####\n");
                    SaveLog("DataProgram Rcvlog() : <RcvlogResult> not found", st_time);
                    continue;
                }
                sscanf(index, "<RcvlogResult>%02d</RcvlogResult>", &ret);
                printf("ret = %02d\n", ret);
                if ( ret == 0 ) {
                    printf("File %s update OK\n", FILENAME);
                    sprintf(buf, "DataProgram Rcvlog() : update %s OK", FILENAME);
                    SaveLog(buf, st_time);
                    clean = 0;
                    upok++;
                } else {
                    printf("File %s update Fail\n", FILENAME);
                    sprintf(buf, "DataProgram Rcvlog() : update %s Fail", FILENAME);
                    SaveLog(buf, st_time);
                    clean = 1;
                    if ( stat(FILENAME, &st) == 0 )
                        if ( st.st_size <= 20 ) {
                            remove(FILENAME);
                            printf("remove empty file!\n");
                        }
                }
                cnt++;
                if ( cnt == UPDATE_MAX ) {
                    printf("Update count = %d, Exit Rcvlog!\n", cnt);
                    fclose(ptime_fd);
                    fclose(pdate_fd);
                    return 0;
                }
            }
            fclose(ptime_fd);
        }
        fclose(pdate_fd);

        if ( upok )
            break;
    }

    // upload /run/user/1000/tmplog if upok==0
    if ( !upok ) {
        // set curl file
        pcurlfile_fd = fopen(CURL_FILE, "wb");
        if ( pcurlfile_fd == NULL ) {
            printf("#### Open %s Fail ####\n", CURL_FILE);
        } else {
            fputs(SOAP_HEAD, pcurlfile_fd);
            sprintf(buf, "\t\t<Rcvlog xmlns=\"http://tempuri.org/\">\n");
            fputs(buf, pcurlfile_fd);
            sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
            fputs(buf, pcurlfile_fd);
            sprintf(buf, "\t\t\t<devicefile>");
            fputs(buf, pcurlfile_fd);
            base64_encode("/run/user/1000/tmplog", pcurlfile_fd);
            sprintf(buf, "</devicefile>\n");
            fputs(buf, pcurlfile_fd);
            sprintf(buf, "\t\t</Rcvlog>\n");
            fputs(buf, pcurlfile_fd);
            fputs(SOAP_TAIL, pcurlfile_fd);
            fclose(pcurlfile_fd);

            // run curl soap to web server
            sprintf(buf, "%s > /run/user/1000/Rcvlog", g_CURL_CMD);
            system(buf);

            // check responds
            if ( stat("/run/user/1000/Rcvlog", &st) == 0 )
                if ( st.st_size == 0 ) {
                    printf("#### Rcvlog() /run/user/1000/Rcvlog empty ####\n");
                    SaveLog("DataProgram Rcvlog() : /run/user/1000/Rcvlog empty", st_time);
                    return 4;
                }

            // read data
            presult_fd = fopen("/run/user/1000/Rcvlog", "rb");
            if ( presult_fd == NULL ) {
                printf("#### Open /run/user/1000/Rcvlog Fail ####\n");
            } else {
                memset(buf, 0, 512);
                fread(buf, 1, 512, presult_fd);
                fclose(presult_fd);
                //printf("/run/user/1000/Rcvlog : \n%s\n", buf);

                index = strstr(buf, "<RcvlogResult>");
                if ( index == NULL ) {
                    printf("#### Rcvlog() <RcvlogResult> not found ####\n");
                    SaveLog("DataProgram Rcvlog() : <RcvlogResult> not found", st_time);
                } else {
                    sscanf(index, "<RcvlogResult>%02d</RcvlogResult>", &ret);
                    printf("ret = %02d\n", ret);
                    if ( ret == 0 ) {
                        printf("File /run/user/1000/tmplog update OK\n");
                        SaveLog("DataProgram Rcvlog() : update /run/user/1000/tmplog OK", st_time);
                    } else {
                        printf("File /run/user/1000/tmplog update Fail\n");
                        SaveLog("DataProgram Rcvlog() : update /run/user/1000/tmplog Fail", st_time);
                    }
                }
            }
        }
    }

    printf("======================== Rcvlog end =========================\n");
    return 0;
}

int Qryerrlogdata()
{
    // Qryerrlogdata result date length about = 401
    char buf[512] = {0};
    char *index = NULL;
    FILE *fd = NULL;
    time_t current_time;
    time_t data_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);


    printf("======================== Qryerrlogdata start ========================\n");
    // set Qryerrlogdata xml file
    fd = fopen(CURL_FILE, "wb");
    if ( fd == NULL ) {
        printf("#### Qryerrlogdata() open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    fputs(SOAP_HEAD, fd);
    sprintf(buf, "\t\t<Qryerrlogdata xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, fd);
    sprintf(buf, "\t\t</Qryerrlogdata>\n");
    fputs(buf, fd);
    fputs(SOAP_TAIL, fd);
    fclose(fd);

    // run curl soap command, save result to /run/user/1000/Qryerrlogdata
    sprintf(buf, "%s > /run/user/1000/Qryerrlogdata", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/Qryerrlogdata", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### Qryerrlogdata() /run/user/1000/Qryerrlogdata empty ####\n");
            SaveLog("DataProgram Qryerrlogdata() : /run/user/1000/Qryerrlogdata empty", st_time);
            return 2;
        }
    // read result
    fd = fopen("/run/user/1000/Qryerrlogdata", "rb");
    if ( fd == NULL ) {
        printf("#### Qryerrlogdata() open /run/user/1000/Qryerrlogdata Fail ####\n");
        return 3;
    }
    memset(buf, 0, 512);
    fread(buf, 1, 512, fd);
    fclose(fd);
    //printf("/run/user/1000/Qryerrlogdata : \n%s\n", buf);

    // set time
    index = strstr(buf, "<QryerrlogdataResult>");
    if ( index == NULL ) {
        printf("#### Qryerrlogdata() <QryerrlogdataResult> not found ####\n");
        SaveLog("DataProgram Qryerrlogdata() : <QryerrlogdataResult> not found", st_time);
        index = strstr(buf, "<QryerrlogdataResult />");
        if ( index == NULL ) {
            printf("#### Qryerrlogdata() <QryerrlogdataResult /> not found ####\n");
            st_errlog_time.tm_year = 0;
            st_errlog_time.tm_mon = 0;
            st_errlog_time.tm_mday = 0;
            st_errlog_time.tm_hour = 0;
            st_errlog_time.tm_min = 0;
            st_errlog_time.tm_sec = 0;
            printf("========================= Qryerrlogdata end =========================\n");
            return 4;
        }
        else {
            printf("<QryerrlogdataResult /> find, last time data not exist!\n");
            SaveLog("DataProgram Qryerrlogdata() : last time data not exist", st_time);
            //data_time = current_time - (update_interval*60);
            data_time = current_time - (60*60);
            st_errlog_time = *localtime(&data_time);
            printf("Set Qrylogdata time : %04d/%02d/%02d %02d:%02d:%02d\n",
            st_errlog_time.tm_year+1900, st_errlog_time.tm_mon+1, st_errlog_time.tm_mday, st_errlog_time.tm_hour, st_errlog_time.tm_min, st_errlog_time.tm_sec);
            printf("========================= Qryerrlogdata end =========================\n");
            return 0;
        }
    }

    sscanf(index, "<QryerrlogdataResult>%04d/%02d/%02d %02d:%02d:%02d</QryerrlogdataResult>",
           &st_errlog_time.tm_year, &st_errlog_time.tm_mon, &st_errlog_time.tm_mday, &st_errlog_time.tm_hour, &st_errlog_time.tm_min, &st_errlog_time.tm_sec);
    st_errlog_time.tm_year -= 1900;
    st_errlog_time.tm_mon -= 1;
    printf("Get Qryerrlogdata time : %04d/%02d/%02d %02d:%02d:%02d\n",
            st_errlog_time.tm_year+1900, st_errlog_time.tm_mon+1, st_errlog_time.tm_mday, st_errlog_time.tm_hour, st_errlog_time.tm_min, st_errlog_time.tm_sec);

    printf("======================= Qryerrlogdata end =======================\n");
    sprintf(buf, "DataProgram Qryerrlogdata() : %04d/%02d/%02d %02d:%02d:%02d",
            st_errlog_time.tm_year+1900, st_errlog_time.tm_mon+1, st_errlog_time.tm_mday, st_errlog_time.tm_hour, st_errlog_time.tm_min, st_errlog_time.tm_sec);
    SaveLog(buf, st_time);
    return 0;
}

int Rcverrorlog()
{
    char FILENAME[64] = {0};
    char buf[512] = {0}; // Rcvlog result date length about = 376
    FILE *pdate_fd = NULL;
    FILE *ptime_fd = NULL;
    FILE *pcurlfile_fd = NULL;
    FILE *presult_fd = NULL;
    struct tm st_file_time = {0};
    char *index = NULL;
    int ret = 0;
    int errlogdate = 0;
    int qryerrlogdate = 0;
    int cnt = 0;
    struct stat st;
    int clean = 0, upok = 0;
    time_t current_time;
    struct tm *st_time;
    int i = 0, j = 0;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("====================== Rcverrorlog start ======================\n");
    if ( strstr(g_ERRLOG_PATH, g_USB_PATH) )
        j = 2; // storage & /run/user/1000
    else
        j = 1; // only /run/user/1000

    for ( i = 0; i < j; i++ ) {
        if ( i == 1 ) // change path from storage to /run/user/1000
            strcpy(g_ERRLOG_PATH, ERRLOG_XML_PATH);

        // get Log file date list
        sprintf(buf, "cd %s; ls > /run/user/1000/ErrLogDate", g_ERRLOG_PATH);
        //printf("buf cmd = %s\n", buf);
        system(buf);

        pdate_fd = fopen("/run/user/1000/ErrLogDate", "rb");
        if ( pdate_fd == NULL ) {
            printf("#### Open /run/user/1000/ErrLogDate Fail ####\n");
            return 1;
        }

        qryerrlogdate = (st_errlog_time.tm_year+1900)*10000 + (st_errlog_time.tm_mon+1)*100 + st_errlog_time.tm_mday;
        //printf("QryErrLogDate = %d\n", qryerrlogdate);

        // get file date
        memset(buf, 0, 512);
        while ( fgets(buf, 64, pdate_fd) != NULL ) {
            if ( strlen(buf) == 0 )
                break;
            // set '\n' to 0
            buf[strlen(buf)-1] = 0;
            sscanf(buf, "%d", &errlogdate);
            //printf("Get ErrLogDate = %d\n", errlogdate);
            if ( errlogdate < qryerrlogdate ) {
                //printf("Old date, continue\n");
                if ( strstr(g_ERRLOG_PATH, ERRLOG_XML_PATH) != NULL ) {
                    sprintf(buf, "rm -rf %s/%d", g_ERRLOG_PATH, errlogdate);
                    //printf("cmd = %s\n", buf);
                    system(buf);
                }
                continue;
            }
            sprintf(buf, "cd %s/%d; ls > /run/user/1000/ErrLogTime", g_ERRLOG_PATH, errlogdate);
            //printf("buf cmd = %s\n", buf);
            system(buf);

            ptime_fd = fopen("/run/user/1000/ErrLogTime", "rb");
            if ( ptime_fd == NULL ) {
                printf("#### Open /run/user/1000/ErrLogTime Fail ####\n");
                return 2;
            }
            // get file time
            memset(buf, 0, 512);
            while ( fgets(buf, 64, ptime_fd) != NULL ) {
                if ( strlen(buf) == 0 )
                    break;
                // set '\n' to 0
                buf[strlen(buf)-1] = 0;

                // remove data if save as host (ex. /run/user/1000)
                if ( strstr(FILENAME, ERRLOG_XML_PATH) )
                    remove(FILENAME);
                // remove empty file
                if ( stat(FILENAME, &st) == 0 )
                    if ( st.st_size <= 20 ) {
                            remove(FILENAME);
                            printf("remove empty file!\n");
                    }
                // clean file
                if ( clean )
                    remove(FILENAME);

                sprintf(FILENAME, "%s/%d/%s", g_ERRLOG_PATH, errlogdate, buf);
                //printf("FILENAME = %s\n", FILENAME);
                // get file time
                st_file_time.tm_year = errlogdate/10000 - 1900;
                st_file_time.tm_mon = (errlogdate%10000)/100 - 1;
                st_file_time.tm_mday = errlogdate%100;
                sscanf(buf, "%02d%02d%02d", &st_file_time.tm_hour, &st_file_time.tm_min, &st_file_time.tm_sec);
                //st_file_time.tm_sec = 0;
                //printf("File time : %04d/%02d/%02d %02d:%02d\n",
                //    st_file_time.tm_year+1900, st_file_time.tm_mon+1, st_file_time.tm_mday, st_file_time.tm_hour, st_file_time.tm_min);
                // check time,
                if ( !CheckTime(st_errlog_time, st_file_time, 0) ) {
                    //printf("Old file %s\n", FILENAME);
                    //printf("Not to do update\n");
                    //if ( !CheckTime(st_errlog_time, st_file_time, save_time) ) {
                    //    printf("File too old, remove file %s\n", FILENAME);
                    //    remove(FILENAME);
                    //}
                    continue;
                }
                // check file
                if ( stat(FILENAME, &st) == 0 )
                    if ( st.st_size <= 20 ) {
                        printf("empty file!\n");
                        clean = 1;
                        continue;
                    }

                // set curl file
                pcurlfile_fd = fopen(CURL_FILE, "wb");
                if ( pcurlfile_fd == NULL ) {
                    printf("#### Open %s Fail ####\n", CURL_FILE);
                    continue;
                }
                fputs(SOAP_HEAD, pcurlfile_fd);
                sprintf(buf, "\t\t<Rcverrorlog xmlns=\"http://tempuri.org/\">\n");
                fputs(buf, pcurlfile_fd);
                sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
                fputs(buf, pcurlfile_fd);
                sprintf(buf, "\t\t\t<devicefile>");
                fputs(buf, pcurlfile_fd);
                base64_encode(FILENAME, pcurlfile_fd);
                sprintf(buf, "</devicefile>\n");
                fputs(buf, pcurlfile_fd);
                sprintf(buf, "\t\t</Rcverrorlog>\n");
                fputs(buf, pcurlfile_fd);
                fputs(SOAP_TAIL, pcurlfile_fd);
                fclose(pcurlfile_fd);

                // run curl soap to web server
                sprintf(buf, "%s > /run/user/1000/Rcverrorlog", g_CURL_CMD);
                system(buf);

                // check responds
                if ( stat("/run/user/1000/Rcverrorlog", &st) == 0 )
                    if ( st.st_size == 0 ) {
                        printf("#### Rcverrorlog() /run/user/1000/Rcverrorlog empty ####\n");
                        SaveLog("DataProgram Rcverrorlog() : /run/user/1000/Rcverrorlog empty", st_time);
                        //continue;
                        fclose(ptime_fd);
                        fclose(pdate_fd);
                        return 3;
                    }
                // read data
                presult_fd = fopen("/run/user/1000/Rcverrorlog", "rb");
                if ( presult_fd == NULL ) {
                    printf("#### Open /run/user/1000/Rcverrorlog Fail ####\n");
                    continue;
                }
                memset(buf, 0, 512);
                fread(buf, 1, 512, presult_fd);
                fclose(presult_fd);
                //printf("/run/user/1000/Rcverrorlog : \n%s\n", buf);

                index = strstr(buf, "<RcverrorlogResult>");
                if ( index == NULL ) {
                    printf("#### Rcverrorlog() <RcverrorlogResult> not found ####\n");
                    SaveLog("DataProgram Rcverrorlog() : <RcverrorlogResult> not found", st_time);
                    continue;
                }
                sscanf(index, "<RcverrorlogResult>%02d</RcverrorlogResult>", &ret);
                printf("ret = %02d\n", ret);
                if ( ret == 0 ) {
                    printf("File %s update OK\n", FILENAME);
                    sprintf(buf, "DataProgram Rcverrorlog() : update %s OK", FILENAME);
                    SaveLog(buf, st_time);
                    clean = 0;
                    upok++;
                } else {
                    printf("File %s update Fail\n", FILENAME);
                    sprintf(buf, "DataProgram Rcverrorlog() : update %s Fail", FILENAME);
                    SaveLog(buf, st_time);
                    clean = 1;
                    if ( stat(FILENAME, &st) == 0 )
                        if ( st.st_size <= 20 ) {
                            remove(FILENAME);
                            printf("remove empty file!\n");
                        }
                }
                cnt++;
                if ( cnt == UPDATE_MAX ) {
                    printf("Update count = %d, Exit Rcverrorlog!\n", cnt);
                    fclose(ptime_fd);
                    fclose(pdate_fd);
                    return 0;
                }
            }
            fclose(ptime_fd);
        }
        fclose(pdate_fd);

        if ( upok )
            break;
    }

    // upload /run/user/1000/tmperrlog if upok==0
    if ( !upok ) {
        // set curl file
        pcurlfile_fd = fopen(CURL_FILE, "wb");
        if ( pcurlfile_fd == NULL ) {
            printf("#### Open %s Fail ####\n", CURL_FILE);
        } else {
            fputs(SOAP_HEAD, pcurlfile_fd);
            sprintf(buf, "\t\t<Rcverrorlog xmlns=\"http://tempuri.org/\">\n");
            fputs(buf, pcurlfile_fd);
            sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
            fputs(buf, pcurlfile_fd);
            sprintf(buf, "\t\t\t<devicefile>");
            fputs(buf, pcurlfile_fd);
            base64_encode("/run/user/1000/tmperrlog", pcurlfile_fd);
            sprintf(buf, "</devicefile>\n");
            fputs(buf, pcurlfile_fd);
            sprintf(buf, "\t\t</Rcverrorlog>\n");
            fputs(buf, pcurlfile_fd);
            fputs(SOAP_TAIL, pcurlfile_fd);
            fclose(pcurlfile_fd);

            // run curl soap to web server
            sprintf(buf, "%s > /run/user/1000/Rcverrorlog", g_CURL_CMD);
            system(buf);

            // check responds
            if ( stat("/run/user/1000/Rcverrorlog", &st) == 0 )
                if ( st.st_size == 0 ) {
                    printf("#### Rcverrorlog() /run/user/1000/Rcverrorlog empty ####\n");
                    SaveLog("DataProgram Rcverrorlog() : /run/user/1000/Rcverrorlog empty", st_time);
                    return 4;
                }

            // read data
            presult_fd = fopen("/run/user/1000/Rcverrorlog", "rb");
            if ( presult_fd == NULL ) {
                printf("#### Open /run/user/1000/Rcverrorlog Fail ####\n");
            } else {
                memset(buf, 0, 512);
                fread(buf, 1, 512, presult_fd);
                fclose(presult_fd);
                //printf("/run/user/1000/Rcverrorlog : \n%s\n", buf);

                index = strstr(buf, "<RcverrorlogResult>");
                if ( index == NULL ) {
                    printf("#### Rcverrorlog() <RcverrorlogResult> not found ####\n");
                    SaveLog("DataProgram Rcverrorlog() : <RcverrorlogResult> not found", st_time);
                } else {
                    sscanf(index, "<RcverrorlogResult>%02d</RcverrorlogResult>", &ret);
                    printf("ret = %02d\n", ret);
                    if ( ret == 0 ) {
                        printf("File /run/user/1000/Rcverrorlog update OK\n");
                        SaveLog("DataProgram Rcverrorlog() : update /run/user/1000/Rcverrorlog OK", st_time);
                    } else {
                        printf("File /run/user/1000/Rcverrorlog update Fail\n");
                        SaveLog("DataProgram Rcverrorlog() : update /run/user/1000/Rcverrorlog Fail", st_time);
                    }
                }
            }
        }
    }

    printf("======================= Rcverrorlog end =======================\n");
    return 0;
}

int Updsampletime()
{
    // Updsampletime result date length about = 396
    char buf[512] = {0};
    char *index = NULL;
    FILE *fd = NULL;
    int tmp_interval = 0;
    time_t current_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("====================== Updsampletime start ======================\n");
    // get interval in init file
    fd = popen("/home/linaro/bin/parameter.sh get sample_time", "r");
    if ( fd == NULL ) {
        printf("popen fail!\n");
        return 1;
    }
    fgets(buf, 512, fd);
    pclose(fd);

    sscanf(buf, "%d", &tmp_interval);
    //printf("tmp_interval = %d\n", tmp_interval);

    if ( data_interval == tmp_interval ) {
        printf("same data interval %d\n", tmp_interval);
        printf("======================= Updsampletime end =======================\n");
        return 2;
    }
    data_interval = tmp_interval;

    // set Updsampletime xml file
    fd = fopen(CURL_FILE, "wb");
    if ( fd == NULL ) {
        printf("#### Updsampletime() open %s Fail ####\n", CURL_FILE);
        return 3;
    }
    memset(buf, 0, 512);
    fputs(SOAP_HEAD, fd);
    sprintf(buf, "\t\t<Updsampletime xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<samplingtime>%d</samplingtime>\n", data_interval);
    fputs(buf, fd);
    sprintf(buf, "\t\t</Updsampletime>\n");
    fputs(buf, fd);
    fputs(SOAP_TAIL, fd);
    fclose(fd);

    // run curl soap command, save result to /run/user/1000/Updsampletime
    sprintf(buf, "%s > /run/user/1000/Updsampletime", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/Updsampletime", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### Updsampletime() /run/user/1000/Updsampletime empty ####\n");
            SaveLog("DataProgram Updsampletime() : /run/user/1000/Updsampletime empty", st_time);
            return 4;
        }
    // read result
    fd = fopen("/run/user/1000/Updsampletime", "rb");
    if ( fd == NULL ) {
        printf("#### Updsampletime() open /run/user/1000/Updsampletime Fail ####\n");
        return 5;
    }
    memset(buf, 0, 512);
    fread(buf, 1, 512, fd);
    fclose(fd);
    //printf("/run/user/1000/Updsampletime : \n%s\n", buf);

    // set time
    index = strstr(buf, "<UpdsampletimeResult>");
    if ( index == NULL ) {
        printf("#### Updsampletime() <UpdsampletimeResult> not found ####\n");
        SaveLog("DataProgram Updsampletime() : <UpdsampletimeResult> not found", st_time);
        index = strstr(buf, "<UpdsampletimeResult />");
        if ( index == NULL )
            printf("#### Updsampletime() <UpdsampletimeResult /> not found ####\n");
        else {
            printf("<UpdsampletimeResult /> find, result data not exist!\n");
            SaveLog("DataProgram Updsampletime() : result data not exist", st_time);
        }
        return 6;
    }

    if ( strstr(buf, "false") ) {
        SaveLog("DataProgram Updsampletime() : result false", st_time);
        return 7;
    }

    if ( !strstr(buf, "true") )
        return 8;

    printf("======================= Updsampletime end =======================\n");
    SaveLog("DataProgram Updsampletime() : OK", st_time);
    return 0;
}

int CheckUpdateInterval()
{
    char buf[512] = {0};
    FILE *fd = NULL;
    int tmp_interval = 0;

    printf("====================== CheckUpdateInterval start ======================\n");
    // get update interval
    fd = popen("/home/linaro/bin/parameter.sh get upload_time", "r");
    if ( fd == NULL ) {
        printf("popen fail!\n");
        return 1;
    }
    fgets(buf, 512, fd);
    pclose(fd);

    sscanf(buf, "%d", &tmp_interval);

    if ( update_interval == tmp_interval ) {
        printf("same update interval %d\n", tmp_interval);
        printf("======================= CheckUpdateInterval end =======================\n");
        return 2;
    }
    update_interval = tmp_interval;

    printf("======================= CheckUpdateInterval end =======================\n");

    return 0;
}

int Updheartbeattime(time_t time)
{
    // Updheartbeattime result date length about = 396
    char buf[512] = {0};
    char timebuf[32] = {0};
    char *index = NULL;
    FILE *fd = NULL;
    struct tm *st_time;
    int ret = 0;
    struct stat st;

    printf("====================== Updheartbeattime start ======================\n");

    // get local time
    st_time = localtime(&time);
    sprintf(timebuf, "%4d-%02d-%02d %02d:%02d:%02d", 1900+st_time->tm_year, 1+st_time->tm_mon, st_time->tm_mday, st_time->tm_hour, st_time->tm_min, st_time->tm_sec);
    printf("localtime : %s\n", timebuf);

    // set Updheartbeattime xml file
    fd = fopen(CURL_FILE, "wb");
    if ( fd == NULL ) {
        printf("#### Updheartbeattime() open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    memset(buf, 0, 512);
    fputs(SOAP_HEAD, fd);
    sprintf(buf, "\t\t<UpdheartbeattimeV2 xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<hearttime>%s</hearttime>\n", timebuf);
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<Ver>%s</Ver>\n", VERSION);
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<SWVer>%s</SWVer>\n", SWVER);
    fputs(buf, fd);
    sprintf(buf, "\t\t</UpdheartbeattimeV2>\n");
    fputs(buf, fd);
    fputs(SOAP_TAIL, fd);
    fclose(fd);

    // run curl soap command, save result to /run/user/1000/Updheartbeattime
    sprintf(buf, "%s > /run/user/1000/Updheartbeattime", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/Updheartbeattime", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### Updheartbeattime() /run/user/1000/Updheartbeattime empty ####\n");
            SaveLog("DataProgram Updheartbeattime() : /run/user/1000/Updheartbeattime empty", st_time);
            return 2;
        }
    // read result
    fd = fopen("/run/user/1000/Updheartbeattime", "rb");
    if ( fd == NULL ) {
        printf("#### Updheartbeattime() open /run/user/1000/Updheartbeattime Fail ####\n");
        return 3;
    }
    memset(buf, 0, 512);
    fread(buf, 1, 512, fd);
    fclose(fd);
    //printf("/run/user/1000/Updheartbeattime : \n%s\n", buf);

    // check result
    index = strstr(buf, "<UpdheartbeattimeV2Result>");
    if ( index == NULL ) {
        printf("#### Updheartbeattime() <UpdheartbeattimeV2Result> not found ####\n");
        SaveLog("DataProgram Updheartbeattime() : <UpdheartbeattimeV2Result> not found", st_time);
        index = strstr(buf, "<UpdheartbeattimeV2Result />");
        if ( index == NULL )
            printf("#### Updheartbeattime() <UpdheartbeattimeV2Result /> not found ####\n");
        else {
            printf("<UpdheartbeattimeV2Result /> find, result data not exist!\n");
            SaveLog("DataProgram Updheartbeattime() : result data not exist", st_time);
        }

        return 4;
    }
    sscanf(index, "<UpdheartbeattimeV2Result>%02d</UpdheartbeattimeV2Result>", &ret);
    printf("ret = %02d\n", ret);
    if ( ret == 0 ) {
        printf("Updheartbeattime update OK\n");
        SaveLog("DataProgram Updheartbeattime() : update OK", st_time);
        printf("======================= Updheartbeattime end =======================\n");
        return 0;
    } else {
        printf("Updheartbeattime update Fail\n");
        SaveLog("DataProgram Updheartbeattime() : update Fail", st_time);
        printf("======================= Updheartbeattime end =======================\n");
        return 5;
    }
}

int GetMIList()
{
    char buf[256] = {0}, manufacturer[64] = {0}, model[64] = {0};
    FILE *fd = NULL;
    int size = 0, inlen = 0, outlen = 0, slaveid = 0, devid = 0, comport = 0, tmp_slaveid = 256, tmp_devid = 256, tmp_comport = 256;
    char *data = NULL, *start_index = NULL, *end_index = NULL, *data_index = NULL;
    unsigned char *decode_data = NULL;
    char *comport_index = NULL, *slaveid_index = NULL, *devid_index = NULL;
    char *manufacturer_index_s = NULL, *manufacturer_index_e = NULL, *model_index_s = NULL, *model_index_e = NULL;
    time_t current_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    init();
    getConfig();
    setCMD();
    setPath();
    OpenLog(g_SYSLOG_PATH, st_time);
    SaveLog("DataProgram GetMIList() : start", st_time);

    system("rm /run/user/1000/GetMIList; sync");

    printf("====================== GetMIList start ======================\n");

    // set GetMIList xml file
    fd = fopen(CURL_FILE, "wb");
    if ( fd == NULL ) {
        printf("#### GetMIList() open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    fputs(SOAP_HEAD, fd);
    sprintf(buf, "\t\t<GetMIList xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, fd);
    sprintf(buf, "\t\t</GetMIList>\n");
    fputs(buf, fd);
    fputs(SOAP_TAIL, fd);
    fclose(fd);

    // run curl soap command, save result to /run/user/1000/GetMIList
    sprintf(buf, "%s > /run/user/1000/GetMIList", g_CURL_CMD);
    system(buf);

     // check responds
    if ( stat("/run/user/1000/GetMIList", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### GetMIList() /run/user/1000/GetMIList empty ####\n");
            SaveLog("DataProgram GetMIList() : /run/user/1000/GetMIList empty", st_time);
            return 2;
        }

    // read data
    fd = fopen("/run/user/1000/GetMIList", "rb");
    if ( fd == NULL ) {
        printf("#### GetMIList() open /run/user/1000/GetMIList Fail ####\n");
        return 3;
    }
    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    //printf("size = %d\n", size);
    // read result
    data = (char*)malloc(size+1);
    memset(data, 0x00, size+1);
    fread(data, 1, size, fd);
    fclose(fd);
    //printf("data : \n%s\n", data);

    // find start address & length
    start_index = strstr(data, "<GetMIListResult>");
    if ( start_index == NULL ) {
        printf("#### GetMIList() <GetMIListResult> not found ####\n");
        SaveLog("DataProgram GetMIList() : <GetMIListResult> not found", st_time);
        if ( data )
            free(data);
        return 4;
    }
    start_index += 17; // <GetMIListResult> length
    end_index = strstr(data, "</GetMIListResult>");
    if ( end_index == NULL ) {
        printf("#### GetMIList() </GetMIListResult> not found ####\n");
        if ( data )
            free(data);
        return 5;
    }

    inlen = end_index - start_index;
    //printf("inlen = %d\n", inlen);

    decode_data = base64_decode(start_index, inlen, &outlen);
    if ( decode_data == NULL ) {
        printf("#### GetMIList() decode_data = NULL ####\n");
        SaveLog("DataProgram GetMIList() : decode_data = NULL", st_time);
        if ( data )
            free(data);
        return 6;
    }
    printf("decode_data len = %d, : \n%s\n", outlen, decode_data);
    SaveLog("DataProgram GetMIList() : OK", st_time);
    CloseLog();

    // make model list
    fd = fopen(MODEL_LIST_PATH, "wb");
    if ( fd == NULL ) {
        printf("#### GetMIList() open MODEL_LIST_PATH Fail ####\n");
        SaveLog("DataProgram GetMIList() : open MODEL_LIST_PATH Fail", st_time);
        if ( data )
            free(data);
        if ( decode_data )
            free(decode_data);
        return 7;
    }
    printf("parser data\n");
    data_index = (char*)decode_data;
    while( data_index != NULL ) {
        memset(manufacturer, 0x00, 64);
        memset(model, 0x00, 64);
        // find com port
        data_index = strstr(data_index, "<port>");
        if ( data_index != NULL )
            comport_index = data_index;
        else {
            printf("<port> not found\n");
            break;
        }

        // find Manufacturer
        data_index = strstr(data_index, "<Manufacturer>");
        if ( data_index != NULL )
            manufacturer_index_s = data_index;
        else {
            printf("<Manufacturer> not found\n");
            break;
        }
        data_index = strstr(data_index, "</Manufacturer>");
        if ( data_index != NULL )
            manufacturer_index_e = data_index;
        else {
            printf("</Manufacturer> not found\n");
            break;
        }

        // find Model
        data_index = strstr(data_index, "<Model>");
        if ( data_index != NULL )
            model_index_s = data_index;
        else {
            printf("<Model> not found\n");
            break;
        }
        data_index = strstr(data_index, "</Model>");
        if ( data_index != NULL )
            model_index_e = data_index;
        else {
            printf("</Model> not found\n");
            break;
        }

        // find slaveId
        data_index = strstr(data_index, "<slaveId>");
        if ( data_index != NULL )
            slaveid_index = data_index;
        else {
            printf("<slaveId> not found\n");
            break;
        }

        // find dev_id
        data_index = strstr(data_index, "<dev_id>");
        if ( data_index != NULL )
            devid_index = data_index;
        else {
            printf("<dev_id> not found\n");
            break;
        }

        if ( (comport_index != NULL) && (manufacturer_index_s != NULL) && (model_index_s != NULL) && (slaveid_index != NULL) && (devid_index != NULL) ) {
            strncpy(manufacturer, manufacturer_index_s+14, manufacturer_index_e-manufacturer_index_s-14);
            //printf("get Manufacturer = %s\n", manufacturer);
            // check Manufacturer
            if ( !strcmp(manufacturer, "DARFON") ) {
                //printf("Match DARFON\n");
                sscanf(comport_index, "<port>COM%d</port>", &comport);
                //printf("comport = %d\n", comport);
                sscanf(slaveid_index, "<slaveId>%d</slaveId>", &slaveid);
                //printf("slaveid = %d\n", slaveid);
                // save minumum slave id setting
                if ( slaveid < tmp_slaveid) {
                    tmp_slaveid = slaveid;
                    tmp_devid = slaveid;
                    tmp_comport = comport;
                }
            } else {
                //printf("Other model\n");
                sscanf(comport_index, "<port>COM%d</port>", &comport);
                //printf("comport = %d\n", comport);
                strncpy(model, model_index_s+7, model_index_e-model_index_s-7);
                //printf("model = %s\n", model);
                sscanf(slaveid_index, "<slaveId>%d</slaveId>", &slaveid);
                //printf("slaveid = %d\n", slaveid);
                sscanf(devid_index, "<dev_id>%d</dev_id>", &devid);
                //printf("devid = %d\n", devid);
                sprintf(buf, "Addr:%03d DEVID:%d Port:COM%d Model:%s\n", slaveid, devid, comport, model);
                fputs(buf, fd);
            }
        }
    }
    //printf("end\n");
    // if have darfon model in list
    if ( tmp_slaveid < 256 ) {
        //printf("tmp_slaveid = %d\n", tmp_slaveid);
        //printf("tmp_devid = %d\n", tmp_devid);
        //printf("tmp_comport = %d\n", tmp_comport);
        sprintf(buf, "Addr:%03d DEVID:%d Port:COM%d Model:Darfon\n", tmp_slaveid, tmp_devid, tmp_comport);
        fputs(buf, fd);
    }
    fclose(fd);

    SaveLog("DataProgram GetMIList() : make model list OK", st_time);

    if ( data )
        free(data);
    if ( decode_data )
        free(decode_data);

    printf("======================= GetMIList end =======================\n");

    return 0;
}

int GetToDoList()
{
    char buf[256] = {0};
    FILE *fd = NULL;
    int size = 0, inlen = 0, outlen = 0;
    char *data = NULL, *start_index = NULL, *end_index = NULL;
    unsigned char *decode_data = NULL;
    time_t current_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("====================== GetToDoList start ======================\n");

    // set GetToDoList xml file
    fd = fopen(CURL_FILE, "wb");
    if ( fd == NULL ) {
        printf("#### GetToDoList() open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    fputs(SOAP_HEAD, fd);
    sprintf(buf, "\t\t<GetToDoList xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, fd);
    sprintf(buf, "\t\t</GetToDoList>\n");
    fputs(buf, fd);
    fputs(SOAP_TAIL, fd);
    fclose(fd);

    // run curl soap command, save result to /run/user/1000/GetToDoList
    sprintf(buf, "%s > /run/user/1000/GetToDoList", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/GetToDoList", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### GetToDoList() /run/user/1000/GetToDoList empty ####\n");
            SaveLog("DataProgram GetToDoList() : /run/user/1000/GetToDoList empty", st_time);
            return 2;
        }
    // read data
    fd = fopen("/run/user/1000/GetToDoList", "rb");
    if ( fd == NULL ) {
        printf("#### GetToDoList() open /run/user/1000/GetToDoList Fail ####\n");
        return 3;
    }
    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    //printf("size = %d\n", size);
    // read result
    data = (char*)malloc(size+1);
    memset(data, 0x00, size+1);
    fread(data, 1, size, fd);
    fclose(fd);
    //printf("data : \n%s\n", data);

    // find start address & length
    start_index = strstr(data, "<GetToDoListResult>");
    if ( start_index == NULL ) {
        printf("#### GetToDoList() <GetToDoListResult> not found ####\n");
        //SaveLog("DataProgram GetToDoList() : <GetToDoListResult> not found", st_time);
        if ( data )
            free(data);
        return 4;
    }
    start_index += 19; // <GetToDoListResult> length
    end_index = strstr(data, "</GetToDoListResult>");
    if ( end_index == NULL ) {
        printf("#### GetToDoList() </GetToDoListResult> not found ####\n");
        if ( data )
            free(data);
        return 5;
    }
    inlen = end_index - start_index;
    //printf("inlen = %d\n", inlen);

    decode_data = base64_decode(start_index, inlen, &outlen);
    if ( decode_data == NULL ) {
        printf("#### GetToDoList() decode_data = NULL ####\n");
        SaveLog("DataProgram GetToDoList() : decode_data = NULL", st_time);
        if ( data )
            free(data);
        return 6;
    }
    //printf("decode_data len = %d, : \n%s\n", outlen, decode_data);
    if ( data )
        free(data);

    // save to do list
    fd = fopen(TODOLIST_PATH, "wb");
    if ( fd == NULL ) {
        printf("#### GetToDoList() open %s Fail ####\n", TODOLIST_PATH);
        return 7;
    }
    memset(buf, 0, 256);
    start_index = (char*)decode_data;
    end_index = (char*)decode_data;
    while ( start_index != NULL && end_index != NULL ) {
        // write sn
        start_index = strstr(start_index, "<sn>");
        end_index = strstr(end_index, "</sn>");
        if ( start_index == NULL || end_index == NULL )
            break;
        fwrite(start_index+4, 1, end_index-(start_index+4), fd);
        fputc(' ', fd);

        // write StartingAddress
        start_index = strstr(start_index, "<StartingAddress>");
        end_index = strstr(end_index, "</StartingAddress>");
        if ( start_index == NULL || end_index == NULL )
            break;
        fwrite(start_index+17, 1, end_index-(start_index+17), fd);
        fputc(' ', fd);

        // write DataCount
        start_index = strstr(start_index, "<DataCount>");
        end_index = strstr(end_index, "</DataCount>");
        if ( start_index == NULL || end_index == NULL )
            break;
        fwrite(start_index+11, 1, end_index-(start_index+11), fd);
        fputc(' ', fd);

        // write CommandValues
        start_index = strstr(start_index, "<CommandValues>");
        end_index = strstr(end_index, "</CommandValues>");
        if ( start_index == NULL || end_index == NULL )
            break;
        fwrite(start_index+15, 1, end_index-(start_index+15), fd);
        fputc('\n', fd);
    }
    fclose(fd);

    if ( decode_data )
        free(decode_data);

    printf("======================= GetToDoList end =======================\n");
    SaveLog("DataProgram GetToDoList() : OK", st_time);
    return 0;
}

int GetWhiteListChanged()
{
    char buf[256] = {0};
    char sn[17] = {0};
    char type[17] = {0};
    FILE *fd = NULL;
    int size = 0, inlen = 0, outlen = 0;
    char *data = NULL, *start_index = NULL, *end_index = NULL;
    unsigned char *decode_data = NULL;
    time_t current_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("====================== GetWhiteListChanged start ======================\n");

    // set GetWhiteListChanged xml file
    fd = fopen(CURL_FILE, "wb");
    if ( fd == NULL ) {
        printf("#### GetWhiteListChanged() open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    fputs(SOAP_HEAD, fd);
    sprintf(buf, "\t\t<GetWhiteListChanged xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, fd);
    sprintf(buf, "\t\t</GetWhiteListChanged>\n");
    fputs(buf, fd);
    fputs(SOAP_TAIL, fd);
    fclose(fd);

    // run curl soap command, save result to /run/user/1000/GetWhiteListChanged
    sprintf(buf, "%s > /run/user/1000/GetWhiteListChanged", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/GetWhiteListChanged", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### GetWhiteListChanged() /run/user/1000/GetWhiteListChanged empty ####\n");
            SaveLog("DataProgram GetWhiteListChanged() : /run/user/1000/GetWhiteListChanged empty", st_time);
            return 2;
        }
    // read data
    fd = fopen("/run/user/1000/GetWhiteListChanged", "rb");
    if ( fd == NULL ) {
        printf("#### GetWhiteListChanged() open /run/user/1000/GetWhiteListChanged Fail ####\n");
        return 3;
    }
    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    //printf("size = %d\n", size);
    // read result
    data = (char*)malloc(size+1);
    memset(data, 0x00, size+1);
    fread(data, 1, size, fd);
    fclose(fd);
    //printf("data : \n%s\n", data);

    // find start address & length
    start_index = strstr(data, "<GetWhiteListChangedResult>");
    if ( start_index == NULL ) {
        printf("#### GetWhiteListChanged() <GetWhiteListChangedResult> not found ####\n");
        //SaveLog("DataProgram GetWhiteListChanged() : <GetWhiteListChangedResult> not found", st_time);
        if ( data )
            free(data);
        return 4;
    }
    start_index += 27; // <GetWhiteListChangedResult> length
    end_index = strstr(data, "</GetWhiteListChangedResult>");
    if ( end_index == NULL ) {
        printf("#### GetWhiteListChanged() </GetWhiteListChangedResult> not found ####\n");
        if ( data )
            free(data);
        return 5;
    }
    inlen = end_index - start_index;
    //printf("inlen = %d\n", inlen);

    decode_data = base64_decode(start_index, inlen, &outlen);
    if ( decode_data == NULL ) {
        printf("#### GetWhiteListChanged() decode_data = NULL ####\n");
        SaveLog("DataProgram GetWhiteListChanged() : decode_data = NULL", st_time);
        if ( data )
            free(data);
        return 6;
    }
    //printf("decode_data len = %d, : \n%s\n", outlen, decode_data);
    if ( data )
        free(data);

    // save to do list
    fd = fopen(WL_CHANGED_PATH, "wb");
    if ( fd == NULL ) {
        printf("#### GetWhiteListChanged() open %s Fail ####\n", WL_CHANGED_PATH);
        return 7;
    }
    memset(buf, 0, 256);
    start_index = (char*)decode_data;
    end_index = (char*)decode_data;
    while ( start_index != NULL && end_index != NULL ) {
        // write sn
        start_index = strstr(start_index, "<sn>");
        end_index = strstr(end_index, "</sn>");
        if ( start_index == NULL || end_index == NULL )
            break;
        memset(sn, 0, 17);
        if ( end_index > (start_index+4) ) {// strlen of sn > 0
            strncpy(sn, start_index+4, end_index-(start_index+4));
            //fwrite(sn, 1, strlen(sn), fd);
            //fputc(' ', fd);
        }

        // write ChangeType
        start_index = strstr(start_index, "<ChangeType>");
        end_index = strstr(end_index, "</ChangeType>");
        if ( start_index == NULL || end_index == NULL )
            break;
        memset(type, 0, 17);
        strncpy(type, start_index+12, end_index-(start_index+12));
        // write type
        fwrite(type, 1, strlen(type), fd);
        if ( strlen(sn) ) {
            // write space
            fputc(' ', fd);
            // write sn
            fwrite(sn, 1, strlen(sn), fd);
        }
        fputc('\n', fd);
    }
    fclose(fd);

    if ( decode_data )
        free(decode_data);

    printf("======================= GetWhiteListChanged end =======================\n");
    SaveLog("DataProgram GetWhiteListChanged() : OK", st_time);
    return 0;
}

int UploadBSM(char *sn, char *date, char *hour)
{
    char FILENAME[64] = {0};
    char buf[512] = {0};
    FILE *file_fd = NULL;
    char *index = NULL;
    int ret = 0;
    time_t  current_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("======================== UploadBSM start ========================\n");
    sprintf(FILENAME, "%s/%s/%s_%s", g_BMS_PATH, date, sn, hour);
    printf("FILENAME = %s\n", FILENAME);

    // set curl file
    file_fd = fopen(CURL_FILE, "wb");
    if ( file_fd == NULL ) {
        printf("#### Open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    fputs(SOAP_HEAD, file_fd);
    sprintf(buf, "\t\t<UploadRawDataFile xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<sn>%s</sn>\n", sn);
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataDate>%s</DataDate>\n", date);
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataHour>%s</DataHour>\n", hour);
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataType>BMS</DataType>\n");
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataFile>");
    fputs(buf, file_fd);
    base64_encode(FILENAME, file_fd);
    sprintf(buf, "</DataFile>\n");
    fputs(buf, file_fd);
    sprintf(buf, "\t\t</UploadRawDataFile>\n");
    fputs(buf, file_fd);
    fputs(SOAP_TAIL, file_fd);
    fclose(file_fd);

    // run curl soap to web server
    sprintf(buf, "%s > /run/user/1000/UploadBSM", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/UploadBSM", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### UploadBSM() /run/user/1000/UploadBSM empty ####\n");
            SaveLog("DataProgram UploadBSM() : /run/user/1000/UploadBSM empty", st_time);
            return 2;
        }
    // read data
    file_fd = fopen("/run/user/1000/UploadBSM", "rb");
    if ( file_fd == NULL ) {
        printf("#### Open /run/user/1000/UploadBSM Fail ####\n");
        return 3;
    }
    memset(buf, 0, 512);
    fread(buf, 1, 512, file_fd);
    fclose(file_fd);
    //printf("/run/user/1000/UploadBSM : \n%s\n", buf);

    index = strstr(buf, "<UploadRawDataFileResult>");
    if ( index == NULL ) {
        printf("#### UploadBSM() <UploadRawDataFileResult> not found ####\n");
        SaveLog("DataProgram UploadBSM() : <UploadRawDataFileResult> not found", st_time);
        return 4;
    }
    sscanf(index, "<UploadRawDataFileResult>%02d</UploadRawDataFileResult>", &ret);
    //printf("ret = %02d\n", ret);

    if ( ret == 0 ) {
        printf("File %s update OK\n", FILENAME);
        sprintf(buf, "DataProgram UploadBSM() : update %s OK", FILENAME);
        SaveLog(buf, st_time);
        printf("========================= UploadBSM end =========================\n");
        return 0;
    } else {
        printf("File %s update Fail\n", FILENAME);
        sprintf(buf, "DataProgram UploadBSM() : update %s Fail", FILENAME);
        SaveLog(buf, st_time);
        printf("========================= UploadBSM end =========================\n");
        return 5;
    }
}

int UploadRAW(char *date, char *hour)
{
    char FILENAME[64] = {0};
    char buf[512] = {0};
    FILE *file_fd = NULL;
    FILE *rawtime_fd = NULL;
    char *index = NULL;
    int ret = 0;
    time_t  current_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("======================== UploadRAW start ========================\n");
    // log data
    sprintf(buf, "cd %s/%s; ls %s*> /run/user/1000/RAWtimeLOG", g_LOG_PATH, date, hour);
    printf("logbuf = %s\n", buf);
    system(buf);
    // errlog data
    sprintf(buf, "cd %s/%s; ls %s*> /run/user/1000/RAWtimeERR", g_ERRLOG_PATH, date, hour);
    printf("errbuf = %s\n", buf);
    system(buf);
    // env data
    sprintf(buf, "cd %s/%s; ls %s*> /run/user/1000/RAWtimeENV", g_ENV_PATH, date, hour);
    printf("envbuf = %s\n", buf);
    system(buf);

    // set curl file
    file_fd = fopen(CURL_FILE, "wb");
    if ( file_fd == NULL ) {
        printf("#### Open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    fputs(SOAP_HEAD, file_fd);
    sprintf(buf, "\t\t<UploadRawDataFile xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, file_fd);
    //sprintf(buf, "\t\t\t<sn>%s</sn>\n", sn);
    //fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataDate>%s</DataDate>\n", date);
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataHour>%s</DataHour>\n", hour);
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataType>RAW</DataType>\n");
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataFile>");
    fputs(buf, file_fd);

    // write base64 raw log data
    rawtime_fd = fopen("/run/user/1000/RAWtimeLOG", "rb");
    if ( rawtime_fd == NULL ) {
        printf("#### Open /run/user/1000/RAWtimeLOG Fail ####\n");
    } else {
        // get file time
        memset(buf, 0, 512);
        while ( fgets(buf, 64, rawtime_fd) != NULL ) {
            if ( strlen(buf) == 0 )
                break;
            // set '\n' to 0
            buf[strlen(buf)-1] = 0;
            // set filename
            sprintf(FILENAME, "%s/%s/%s", g_LOG_PATH, date, buf);
            printf("FILENAME = %s\n", FILENAME);
            // run base64 encode
            base64_encode(FILENAME, file_fd);
        }
        fclose(rawtime_fd);
    }
    // write base64 raw err data
    rawtime_fd = fopen("/run/user/1000/RAWtimeERR", "rb");
    if ( rawtime_fd == NULL ) {
        printf("#### Open /run/user/1000/RAWtimeERR Fail ####\n");
    } else {
        // get file time
        memset(buf, 0, 512);
        while ( fgets(buf, 64, rawtime_fd) != NULL ) {
            if ( strlen(buf) == 0 )
                break;
            // set '\n' to 0
            buf[strlen(buf)-1] = 0;
            // set filename
            sprintf(FILENAME, "%s/%s/%s", g_ERRLOG_PATH, date, buf);
            printf("FILENAME = %s\n", FILENAME);
            // run base64 encode
            base64_encode(FILENAME, file_fd);
        }
        fclose(rawtime_fd);
    }
    // write base64 raw env data
    rawtime_fd = fopen("/run/user/1000/RAWtimeENV", "rb");
    if ( rawtime_fd == NULL ) {
        printf("#### Open /run/user/1000/RAWtimeENV Fail ####\n");
    } else {
        // get file time
        memset(buf, 0, 512);
        while ( fgets(buf, 64, rawtime_fd) != NULL ) {
            if ( strlen(buf) == 0 )
                break;
            // set '\n' to 0
            buf[strlen(buf)-1] = 0;
            // set filename
            sprintf(FILENAME, "%s/%s/%s", g_ENV_PATH, date, buf);
            printf("FILENAME = %s\n", FILENAME);
            // run base64 encode
            base64_encode(FILENAME, file_fd);
        }
        fclose(rawtime_fd);
    }

    sprintf(buf, "</DataFile>\n");
    fputs(buf, file_fd);
    sprintf(buf, "\t\t</UploadRawDataFile>\n");
    fputs(buf, file_fd);
    fputs(SOAP_TAIL, file_fd);
    fclose(file_fd);

    printf("RAW set OK\n");

    // run curl soap to web server
    sprintf(buf, "%s > /run/user/1000/UploadRAW", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/UploadRAW", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### UploadRAW() /run/user/1000/UploadRAW empty ####\n");
            SaveLog("DataProgram UploadRAW() : /run/user/1000/UploadRAW empty", st_time);
            return 2;
        }
    // read data
    file_fd = fopen("/run/user/1000/UploadRAW", "rb");
    if ( file_fd == NULL ) {
        printf("#### Open /run/user/1000/UploadRAW Fail ####\n");
        return 3;
    }
    memset(buf, 0, 512);
    fread(buf, 1, 512, file_fd);
    fclose(file_fd);
    //printf("/run/user/1000/UploadRAW : \n%s\n", buf);

    index = strstr(buf, "<UploadRawDataFileResult>");
    if ( index == NULL ) {
        printf("#### UploadRAW() <UploadRawDataFileResult> not found ####\n");
        SaveLog("DataProgram UploadRAW() : <UploadRawDataFileResult> not found", st_time);
        return 4;
    }
    sscanf(index, "<UploadRawDataFileResult>%02d</UploadRawDataFileResult>", &ret);
    //printf("ret = %02d\n", ret);

    if ( ret == 0 ) {
        printf("RAW update OK\n");
        sprintf(buf, "DataProgram UploadRAW() : update %s OK", FILENAME);
        SaveLog(buf, st_time);
        printf("========================= UploadRAW end =========================\n");
        return 0;
    } else {
        printf("RAW update Fail\n");
        sprintf(buf, "DataProgram UploadRAW() : update %s Fail", FILENAME);
        SaveLog(buf, st_time);
        printf("========================= UploadRAW end =========================\n");
        return 5;
    }
}

int UploadLOG(char *date, char *hour)
{
    char FILENAME[64] = {0};
    char buf[512] = {0};
    FILE *file_fd = NULL;
    char *index = NULL;
    int ret = 0;
    time_t  current_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("======================== UploadLOG start ========================\n");
    sprintf(FILENAME, "%s/%s/%s", g_SYSLOG_PATH, date, hour);
    printf("FILENAME = %s\n", FILENAME);

    // set curl file
    file_fd = fopen(CURL_FILE, "wb");
    if ( file_fd == NULL ) {
        printf("#### Open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    fputs(SOAP_HEAD, file_fd);
    sprintf(buf, "\t\t<UploadRawDataFile xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, file_fd);
    //sprintf(buf, "\t\t\t<sn>%s</sn>\n", sn);
    //fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataDate>%s</DataDate>\n", date);
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataHour>%s</DataHour>\n", hour);
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataType>LOG</DataType>\n");
    fputs(buf, file_fd);
    sprintf(buf, "\t\t\t<DataFile>");
    fputs(buf, file_fd);
    base64_encode(FILENAME, file_fd);
    sprintf(buf, "</DataFile>\n");
    fputs(buf, file_fd);
    sprintf(buf, "\t\t</UploadRawDataFile>\n");
    fputs(buf, file_fd);
    fputs(SOAP_TAIL, file_fd);
    fclose(file_fd);

    // run curl soap to web server
    sprintf(buf, "%s > /run/user/1000/UploadLOG", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/UploadLOG", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### UploadLOG() /run/user/1000/UploadLOG empty ####\n");
            SaveLog("DataProgram UploadLOG() : /run/user/1000/UploadLOG empty", st_time);
            return 2;
        }
    // read data
    file_fd = fopen("/run/user/1000/UploadLOG", "rb");
    if ( file_fd == NULL ) {
        printf("#### Open /run/user/1000/UploadLOG Fail ####\n");
        return 3;
    }
    memset(buf, 0, 512);
    fread(buf, 1, 512, file_fd);
    fclose(file_fd);
    //printf("/run/user/1000/UploadLOG : \n%s\n", buf);

    index = strstr(buf, "<UploadRawDataFileResult>");
    if ( index == NULL ) {
        printf("#### UploadLOG() <UploadRawDataFileResult> not found ####\n");
        SaveLog("DataProgram UploadLOG() : <UploadRawDataFileResult> not found", st_time);
        return 4;
    }
    sscanf(index, "<UploadRawDataFileResult>%02d</UploadRawDataFileResult>", &ret);
    //printf("ret = %02d\n", ret);

    if ( ret == 0 ) {
        printf("File %s update OK\n", FILENAME);
        sprintf(buf, "DataProgram UploadLOG() : update %s OK", FILENAME);
        SaveLog(buf, st_time);
        printf("========================= UploadLOG end =========================\n");
        return 0;
    } else {
        printf("File %s update Fail\n", FILENAME);
        sprintf(buf, "DataProgram UploadLOG() : update %s Fail", FILENAME);
        SaveLog(buf, st_time);
        printf("========================= UploadLOG end =========================\n");
        return 5;
    }
}

int QryRawDataFile()
{
    char buf[256] = {0};
    char sn[17] = {0};
    char type[4] = {0};
    char date[9] = {0};
    char hour[3] = {0};
    FILE *fd = NULL;
    int size = 0, inlen = 0, outlen = 0;
    char *data = NULL, *start_index = NULL, *end_index = NULL;
    unsigned char *decode_data = NULL;
    time_t current_time;
    struct tm *st_time;
    struct stat st;

    current_time = time(NULL);
    st_time = localtime(&current_time);

    printf("====================== QryRawDataFile start ======================\n");

    // set QryRawDataFile xml file
    fd = fopen(CURL_FILE, "wb");
    if ( fd == NULL ) {
        printf("#### QryRawDataFile() open %s Fail ####\n", CURL_FILE);
        return 1;
    }
    fputs(SOAP_HEAD, fd);
    sprintf(buf, "\t\t<QryRawDataFile xmlns=\"http://tempuri.org/\">\n");
    fputs(buf, fd);
    sprintf(buf, "\t\t\t<macaddress>%s</macaddress>\n", MAC);
    fputs(buf, fd);
    sprintf(buf, "\t\t</QryRawDataFile>\n");
    fputs(buf, fd);
    fputs(SOAP_TAIL, fd);
    fclose(fd);

    // run curl soap command, save result to /run/user/1000/QryRawDataFile
    sprintf(buf, "%s > /run/user/1000/QryRawDataFile", g_CURL_CMD);
    system(buf);

    // check responds
    if ( stat("/run/user/1000/QryRawDataFile", &st) == 0 )
        if ( st.st_size == 0 ) {
            printf("#### QryRawDataFile() /run/user/1000/QryRawDataFile empty ####\n");
            SaveLog("DataProgram QryRawDataFile() : /run/user/1000/QryRawDataFile empty", st_time);
            return 2;
        }
    // read data
    fd = fopen("/run/user/1000/QryRawDataFile", "rb");
    if ( fd == NULL ) {
        printf("#### QryRawDataFile() open /run/user/1000/QryRawDataFile Fail ####\n");
        return 3;
    }
    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    //printf("size = %d\n", size);
    // read result
    data = (char*)malloc(size+1);
    memset(data, 0x00, size+1);
    fread(data, 1, size, fd);
    fclose(fd);
    //printf("data : \n%s\n", data);

    // find start address & length
    start_index = strstr(data, "<QryRawDataFileResult>");
    if ( start_index == NULL ) {
        printf("#### QryRawDataFile() <QryRawDataFileResult> not found ####\n");
        //SaveLog("DataProgram QryRawDataFile() : <QryRawDataFileResult> not found", st_time);
        if ( data )
            free(data);
        return 4;
    }
    start_index += 22; // <QryRawDataFileResult> length
    end_index = strstr(data, "</QryRawDataFileResult>");
    if ( end_index == NULL ) {
        printf("#### QryRawDataFile() </QryRawDataFileResult> not found ####\n");
        if ( data )
            free(data);
        return 5;
    }
    inlen = end_index - start_index;
    //printf("inlen = %d\n", inlen);

    decode_data = base64_decode(start_index, inlen, &outlen);
    if ( decode_data == NULL ) {
        printf("#### QryRawDataFile() decode_data = NULL ####\n");
        SaveLog("DataProgram QryRawDataFile() : decode_data = NULL", st_time);
        if ( data )
            free(data);
        return 6;
    }
    //printf("decode_data len = %d, : \n%s\n", outlen, decode_data);
    if ( data )
        free(data);

    // call update function
    start_index = (char*)decode_data;
    end_index = (char*)decode_data;
    while ( start_index != NULL && end_index != NULL ) {
        memset(sn, 0, 17);
        memset(type, 0, 4);
        memset(date, 0, 9);
        memset(hour, 0, 3);
        // set sn
        start_index = strstr(start_index, "<sn>");
        end_index = strstr(end_index, "</sn>");
        if ( start_index == NULL || end_index == NULL )
            break;
        strncpy(sn, start_index+4, end_index-(start_index+4));

        // set DataType
        start_index = strstr(start_index, "<DataType>");
        end_index = strstr(end_index, "</DataType>");
        if ( start_index == NULL || end_index == NULL )
            break;
        strncpy(type, start_index+10, end_index-(start_index+10));

        // set DataDate
        start_index = strstr(start_index, "<DataDate>");
        end_index = strstr(end_index, "</DataDate>");
        if ( start_index == NULL || end_index == NULL )
            break;
        strncpy(date, start_index+10, end_index-(start_index+10));

        // set  DataHour
        start_index = strstr(start_index, "<DataHour>");
        end_index = strstr(end_index, "</DataHour>");
        if ( start_index == NULL || end_index == NULL )
            break;
        strncpy(hour, start_index+10, end_index-(start_index+10));

        printf("Get SN = %s, Type = %s, Date = %s, Hour = %s\n", sn, type, date, hour);
        if ( strncmp(type, "BMS", 3) == 0 ) {
            //UploadBSM(sn, date, hour);
            sprintf(buf, "%s/bmslist", g_BMS_PATH);
            if ( stat(buf, &st) == 0 )
                fd = fopen(buf, "a");
            else
                fd = fopen(buf, "w");
            fputs(sn, fd);
            fputc('\n', fd);
            fclose(fd);
        } else if ( strncmp(type, "RAW", 3) == 0 )
            UploadRAW(date, hour);
        else if ( strncmp(type, "LOG", 3) == 0 )
            UploadLOG(date, hour);
        else
            printf("unknown type\n");
    }
    if ( decode_data )
        free(decode_data);

    printf("======================= QryRawDataFile end =======================\n");
    SaveLog("DataProgram QryRawDataFile() : OK", st_time);
    return 0;
}

void clean_host_data(time_t time)
{
    char buf[1024] = {0};
    int year = 0, month = 0, day = 0, hour = 0;
    struct tm st_dirtime = {0};
    struct tm *st_systime;
    DIR *dirp;
    DIR *secdirp;
    struct dirent *direntp;
    struct dirent *secdirentp;

    // set time
    st_systime = localtime(&time);

    // check log dir.
    dirp = opendir(LOG_XML_PATH);
    if ( dirp == NULL ) {
        printf("opendir %s fail\n", LOG_XML_PATH);
        //return;
    } else {
        while( (direntp = readdir(dirp)) != NULL ) {
            if ( strlen(direntp->d_name) < 8 ) // skip . & ..
                continue;
            sscanf(direntp->d_name, "%04d%02d%02d", &year, &month, &day);
            st_dirtime.tm_year = year - 1900;
            st_dirtime.tm_mon = month - 1;
            st_dirtime.tm_mday = day;
            st_dirtime.tm_hour = 0;
            st_dirtime.tm_min = 0;
            st_dirtime.tm_sec = 0;
            printf("log st_dirtime = %04d%02d%02d\n", st_dirtime.tm_year+1900, st_dirtime.tm_mon+1, st_dirtime.tm_mday);

            if ( !CheckTime(*st_systime, st_dirtime, 86400) ) {
                // too old, delete
                sprintf(buf, "rm -rf %s/%s", LOG_XML_PATH, direntp->d_name);
                system(buf);
            }
        }
        closedir(dirp);
    }

    // check errlog dir.
    dirp = opendir(ERRLOG_XML_PATH);
    if ( dirp == NULL ) {
        printf("opendir %s fail\n", ERRLOG_XML_PATH);
        //return;
    } else {
        while( (direntp = readdir(dirp)) != NULL ) {
            if ( strlen(direntp->d_name) < 8 ) // skip . & ..
                continue;
            sscanf(direntp->d_name, "%04d%02d%02d", &year, &month, &day);
            st_dirtime.tm_year = year - 1900;
            st_dirtime.tm_mon = month - 1;
            st_dirtime.tm_mday = day;
            st_dirtime.tm_hour = 0;
            st_dirtime.tm_min = 0;
            st_dirtime.tm_sec = 0;
            printf("errlog st_dirtime = %04d%02d%02d\n", st_dirtime.tm_year+1900, st_dirtime.tm_mon+1, st_dirtime.tm_mday);

            if ( !CheckTime(*st_systime, st_dirtime, 86400) ) {
                // too old, delete
                sprintf(buf, "rm -rf %s/%s", ERRLOG_XML_PATH, direntp->d_name);
                system(buf);
            }
        }
        closedir(dirp);
    }

    // check bms dir.
    /*dirp = opendir(BMS_PATH);
    if ( dirp == NULL ) {
        printf("opendir %s fail\n", BMS_PATH);
        //return;
    } else {
        while( (direntp = readdir(dirp)) != NULL ) {
            if ( strlen(direntp->d_name) < 8 ) // skip . & ..
                continue;
            sscanf(direntp->d_name, "%04d%02d%02d", &year, &month, &day);
            st_dirtime.tm_year = year - 1900;
            st_dirtime.tm_mon = month - 1;
            st_dirtime.tm_mday = day;
            st_dirtime.tm_hour = 0;
            st_dirtime.tm_min = 0;
            st_dirtime.tm_sec = 0;
            printf("bms st_dirtime = %04d%02d%02d\n", st_dirtime.tm_year+1900, st_dirtime.tm_mon+1, st_dirtime.tm_mday);

            if ( !CheckTime(*st_systime, st_dirtime, 86400) ) {
                // too old, delete
                sprintf(buf, "rm -rf %s/%s", BMS_PATH, direntp->d_name);
                system(buf);
            } else {
                // today, check hour
                sprintf(buf, "%s/%s", BMS_PATH, direntp->d_name);
                secdirp = opendir(buf);
                if ( secdirp == NULL ) {
                    printf("opendir %s fail\n", buf);
                    //return;
                } else {
                    while( (secdirentp = readdir(secdirp)) != NULL ) {
                        //printf("fine name = %s\n", secdirentp->d_name);
                        if ( secdirentp->d_name[0] == '.' ) // skip . & ..
                            continue;
                        sscanf(secdirentp->d_name, "%16s_%02d", buf, &hour);
                        st_dirtime.tm_hour = hour;
                        printf("bms st_dirtime = %02d\n", st_dirtime.tm_hour);

                        // remove 1 hour ago data
                        if ( !CheckTime(*st_systime, st_dirtime, 3600) ) {
                            // too old, delete
                            sprintf(buf, "rm -f %s/%s/%s", BMS_PATH, direntp->d_name, secdirentp->d_name);
                            system(buf);
                        }
                    }
                    closedir(secdirp);
                }
            }
        }
        closedir(dirp);
    }*/
    if ( (st_systime->tm_hour == 0) && clean_bms_flag ) {
        system("rm -f /run/user/1000/BMS/*; sync");
    }

    // check syslog dir.
    dirp = opendir(SYSLOG_PATH);
    if ( dirp == NULL ) {
        printf("opendir %s fail\n", SYSLOG_PATH);
        //return;
    } else {
        while( (direntp = readdir(dirp)) != NULL ) {
            if ( strlen(direntp->d_name) < 8 ) // skip . & ..
                continue;
            sscanf(direntp->d_name, "%04d%02d%02d", &year, &month, &day);
            st_dirtime.tm_year = year - 1900;
            st_dirtime.tm_mon = month - 1;
            st_dirtime.tm_mday = day;
            st_dirtime.tm_hour = 0;
            st_dirtime.tm_min = 0;
            st_dirtime.tm_sec = 0;
            printf("syslog st_dirtime = %04d%02d%02d\n", st_dirtime.tm_year+1900, st_dirtime.tm_mon+1, st_dirtime.tm_mday);

            if ( !CheckTime(*st_systime, st_dirtime, 86400) ) {
                // too old, delete
                sprintf(buf, "rm -rf %s/%s", SYSLOG_PATH, direntp->d_name);
                system(buf);
            } else {
                // today, check hour
                sprintf(buf, "%s/%s", SYSLOG_PATH, direntp->d_name);
                secdirp = opendir(buf);
                if ( secdirp == NULL ) {
                    printf("opendir %s fail\n", buf);
                    //return;
                } else {
                    while( (secdirentp = readdir(secdirp)) != NULL ) {
                        //printf("fine name = %s\n", secdirentp->d_name);
                        if ( secdirentp->d_name[0] == '.' ) // skip . & ..
                            continue;
                        sscanf(secdirentp->d_name, "%02d", &hour);
                        st_dirtime.tm_hour = hour;
                        printf("syslog st_dirtime = %02d\n", st_dirtime.tm_hour);

                        // remove 2 hours ago data
                        if ( !CheckTime(*st_systime, st_dirtime, 7200) ) {
                            // too old, delete
                            sprintf(buf, "rm -f %s/%s/%s", SYSLOG_PATH, direntp->d_name, secdirentp->d_name);
                            system(buf);
                        }
                    }
                    closedir(secdirp);
                }
            }
        }
        closedir(dirp);
    }

    return;
}

void clean_storage_data(time_t time)
{
    char buf[1024] = {0};
    int year = 0, month = 0, day = 0;
    struct tm st_dirtime = {0};
    struct tm *st_systime;
    struct stat st;
    DIR *dirp;
    struct dirent *direntp;

    // check storage
    if ( stat(USB_DEV, &st) ) {
        if ( stat(SDCARD_PATH, &st) ) {
            printf("Storage device not found!\n");
            return;
        }
    }

    // set time
    st_systime = localtime(&time);
    if ( shelf_life <= 0 )
        shelf_life = 1;

    // check log dir.
    dirp = opendir(g_LOG_PATH);
    if ( dirp == NULL ) {
        printf("opendir %s fail\n", g_LOG_PATH);
        //return;
    } else {
        while( (direntp = readdir(dirp)) != NULL ) {
            if ( strlen(direntp->d_name) < 8 ) // skip . & ..
                continue;
            sscanf(direntp->d_name, "%04d%02d%02d", &year, &month, &day);
            st_dirtime.tm_year = year - 1900;
            st_dirtime.tm_mon = month - 1;
            st_dirtime.tm_mday = day;
            st_dirtime.tm_hour = 0;
            st_dirtime.tm_min = 0;
            st_dirtime.tm_sec = 0;
            printf("log st_dirtime = %04d%02d%02d\n", st_dirtime.tm_year+1900, st_dirtime.tm_mon+1, st_dirtime.tm_mday);

            if ( !CheckTime(*st_systime, st_dirtime, shelf_life*86400) ) {
                // too old, delete
                sprintf(buf, "rm -rf %s/%s", g_LOG_PATH, direntp->d_name);
                system(buf);
            }
        }
        closedir(dirp);
    }

    // check errlog dir.
    dirp = opendir(g_ERRLOG_PATH);
    if ( dirp == NULL ) {
        printf("opendir %s fail\n", g_ERRLOG_PATH);
        //return;
    } else {
        while( (direntp = readdir(dirp)) != NULL ) {
            if ( strlen(direntp->d_name) < 8 ) // skip . & ..
                continue;
            sscanf(direntp->d_name, "%04d%02d%02d", &year, &month, &day);
            st_dirtime.tm_year = year - 1900;
            st_dirtime.tm_mon = month - 1;
            st_dirtime.tm_mday = day;
            st_dirtime.tm_hour = 0;
            st_dirtime.tm_min = 0;
            st_dirtime.tm_sec = 0;
            printf("errlog st_dirtime = %04d%02d%02d\n", st_dirtime.tm_year+1900, st_dirtime.tm_mon+1, st_dirtime.tm_mday);

            if ( !CheckTime(*st_systime, st_dirtime, shelf_life*86400) ) {
                // too old, delete
                sprintf(buf, "rm -rf %s/%s", g_ERRLOG_PATH, direntp->d_name);
                system(buf);
            }
        }
        closedir(dirp);
    }

    // check bms dir.
    /*dirp = opendir(g_BMS_PATH);
    if ( dirp == NULL ) {
        printf("opendir %s fail\n", g_BMS_PATH);
        //return;
    } else {
        while( (direntp = readdir(dirp)) != NULL ) {
            if ( strlen(direntp->d_name) < 8 ) // skip . & ..
                continue;
            sscanf(direntp->d_name, "%04d%02d%02d", &year, &month, &day);
            st_dirtime.tm_year = year - 1900;
            st_dirtime.tm_mon = month - 1;
            st_dirtime.tm_mday = day;
            st_dirtime.tm_hour = 0;
            st_dirtime.tm_min = 0;
            st_dirtime.tm_sec = 0;
            printf("bms st_dirtime = %04d%02d%02d\n", st_dirtime.tm_year+1900, st_dirtime.tm_mon+1, st_dirtime.tm_mday);

            if ( !CheckTime(*st_systime, st_dirtime, shelf_life*86400) ) {
                // too old, delete
                sprintf(buf, "rm -rf %s/%s", g_BMS_PATH, direntp->d_name);
                system(buf);
            }
        }
        closedir(dirp);
    }*/
    if ( (st_systime->tm_hour == 0) && clean_bms_flag ) {
        sprintf(buf, "rm -f %s/*; sync", g_BMS_PATH);
        system(buf);
        clean_bms_flag = 0;
    }

    // check syslog dir.
    dirp = opendir(g_SYSLOG_PATH);
    if ( dirp == NULL ) {
        printf("opendir %s fail\n", g_SYSLOG_PATH);
        //return;
    } else {
        while( (direntp = readdir(dirp)) != NULL ) {
            if ( strlen(direntp->d_name) < 8 ) // skip . & ..
                continue;
            sscanf(direntp->d_name, "%04d%02d%02d", &year, &month, &day);
            st_dirtime.tm_year = year - 1900;
            st_dirtime.tm_mon = month - 1;
            st_dirtime.tm_mday = day;
            st_dirtime.tm_hour = 0;
            st_dirtime.tm_min = 0;
            st_dirtime.tm_sec = 0;
            printf("syslog st_dirtime = %04d%02d%02d\n", st_dirtime.tm_year+1900, st_dirtime.tm_mon+1, st_dirtime.tm_mday);

            if ( !CheckTime(*st_systime, st_dirtime, shelf_life*86400) ) {
                // too old, delete
                sprintf(buf, "rm -rf %s/%s", g_SYSLOG_PATH, direntp->d_name);
                system(buf);
            }
        }
        closedir(dirp);
    }

    return;
}

int main(int argc, char* argv[])
{
    int previous_min = 60;
    int previous_hour = 24;
    int state = 0, ret = -1;
    time_t sys_current_time = 0;
    struct tm *sys_st_time = NULL;

    int opt;
    while( (opt = getopt(argc, argv, "vVtTdD")) != -1 )
    {
        switch (opt)
        {
            case 'v':
            case 'V':
                printf("%s\n", VERSION);
                return 0;
            case 't':
            case 'T':
                printf("Enter test mode\n");
                init();
                // do something for test
                return 0;
            case 'd':
            case 'D':
                printf("Run GetMIList()\n");
                ret = GetMIList();
                //printf("ret = %d\n", ret);
                return ret;
            case '?':
                return 1;
        }
    }

    sys_current_time = time(NULL);
    sys_st_time = localtime(&sys_current_time);
    //previous_hour = sys_st_time->tm_hour;
    //previous_min = sys_st_time->tm_min;

    printf("Data Program Start!\n");
    init();
    getConfig();
    setCMD();
    setPath();
    OpenLog(g_SYSLOG_PATH, sys_st_time);
    SaveLog("DataProgram main() : start", sys_st_time);
    //counter = 9999;
    while (1) {
        // get system time
        sys_current_time = time(NULL);
        sys_st_time = localtime(&sys_current_time);

        // check time to run
        if ( ((update_interval <= 60) && (sys_st_time->tm_sec % update_interval == 0)) ||
            ((update_interval > 60) && (sys_st_time->tm_min % (update_interval/60) == 0) && (previous_min != sys_st_time->tm_min) && (sys_st_time->tm_sec == 0)) ||
            ((update_interval == 3600) && (sys_st_time->tm_min == 0) && (previous_hour != sys_st_time->tm_hour) && (sys_st_time->tm_sec == 0)) ) {

            printf("==== Run upload loop start ====\n");

            // get config & set parameter
            getConfig();
            setCMD();
            setPath();

            // get while list change
            GetWhiteListChanged();

            // get to do list
            GetToDoList();

            if ( state == 0)
                state = 1;
            else if ( state == 1 ) {
                if ( check_milist )
                    CheckWhiteListUploaded();
                else
                    state = 2;
            }

            // update white list if has new file
            PostMIList();

            ret = Qrylogdata();
            if ( ret == 0 ) {
                setPath();
                Rcvlog();
            }

            ret = Qryerrlogdata();
            if ( ret == 0 ) {
                setPath();
                Rcverrorlog();
            }

            setPath();
            QryRawDataFile();

            // check data interval
            if ( !Updsampletime() )
                printf("Updsampletime() change data interval %d OK\n", data_interval);

            // check update interval
            if ( !CheckUpdateInterval() )
                printf("CheckUpdateInterval() change update interval %d OK\n", update_interval);

            // one minute run
            if ( (previous_min != sys_st_time->tm_min) || ((previous_min == sys_st_time->tm_min) && (previous_hour != sys_st_time->tm_hour)) ) {
                previous_min = sys_st_time->tm_min;
                // update alive
                if ( !Updheartbeattime(sys_current_time) )
                    printf("Updheartbeattime() OK\n");

                CloseLog();
                OpenLog(g_SYSLOG_PATH, sys_st_time);
            }

            // one hour run
            if ( previous_hour != sys_st_time->tm_hour ) {
                previous_hour = sys_st_time->tm_hour;
                // clean host data
                clean_host_data(sys_current_time);
                // clean storage data
                clean_storage_data(sys_current_time);
                system("sync");
            }

            printf("==== upload loop end ====\n");
        }

        // sleep
        usleep(100000);

    }

    return 0;
}
