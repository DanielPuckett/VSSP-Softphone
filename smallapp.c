#include <pjlib.h>
#include <pjlib-util.h>
#include <pjmedia.h>
#include <pjmedia-codec.h>
#include <pjsip.h>
#include <pjsip_simple.h>
#include <pjsip_ua.h>

#include <pjsua-lib/pjsua.h>

/* For audio */
#include <pjmedia-audiodev/audiodev.h>
#include <pjmedia-audiodev/audiotest.h>
//#include <pjmedia.h>
//#include <pjlib.h>
//#include <pjlib-util.h>


/* $Id: simple_pjsua.c 2408 2009-01-01 22:08:21Z bennylp $ */
/* 
 * Copyright (C) 2008-2009 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */


/* * * * * * * * * * * * * * * * * * * * 
 *  Parts are copyright 2011           *
 *  Original was about 100 lines       *
 *  Distributel Communications Limited *
 *  dpuckett@distributel.ca            *
 * * * * * * * * * * * * * * * * * * * */ 
//#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>


#define THIS_FILE       "APP"

/* DAP */
static pjsua_call_id    current_call = PJSUA_INVALID_ID;


/* DAP Audio devices */
static void list_devices(void)
{
    static unsigned dev_count;

    unsigned i;
    pj_status_t status;
    
    dev_count = pjmedia_aud_dev_count();
    if (dev_count == 0) {
        PJ_LOG(3,(THIS_FILE, "No devices found"));
        return;
    }
    
    PJ_LOG(3,(THIS_FILE, "Found %d devices:", dev_count));
    
    for (i=0; i<dev_count; ++i) {
        pjmedia_aud_dev_info info;
        
        status = pjmedia_aud_dev_get_info(i, &info);
        if (status != PJ_SUCCESS)
            continue;
        
        PJ_LOG(3,(THIS_FILE," %2d: %s [%s] (%d/%d)",
                  i, info.driver, info.name, info.input_count, info.output_count));
    }
}



static pj_bool_t simple_input(const char *title, char *buf, pj_size_t len)
{
  char *p;
  printf("%s (empty to cancel): ", title); fflush(stdout);
  if (fgets(buf, len, stdin) == NULL)
    return PJ_FALSE;
  /* Remove trailing newlines. */
  for (p=buf; ; ++p)
  {
    if (*p=='\r' || *p=='\n') *p='\0';
    else if (!*p) break;
  }
  if (!*buf) return PJ_FALSE;
  return PJ_TRUE;
}


static int my_atoi(const char *cs)
{
  pj_str_t s;
  pj_cstr(&s, cs);
  if (cs[0] == '-')
  {
    s.ptr++, s.slen--;
    return 0 - (int)pj_strtoul(&s);
  } else if (cs[0] == '+')
  {
    s.ptr++, s.slen--;
    return pj_strtoul(&s);
  } else
  {
    return pj_strtoul(&s);
  }
}


/* DAP Le Wen */
int get_password(const char * const file, char * password)
{
  struct stat buf;
  if(stat(file,&buf)!=0){
    perror("Error by stat\n");
    return -1;
  }
  if( ( (buf.st_mode & 0777) | 0600 ) != 0600 ){
    printf("The password file %s must be user read|write only.\n",file);
    return -1;
  }
  int fd=open(file, O_RDONLY);
  if(fd<0){
    perror("Can not open file\n");
    return -1;
  }
  if(read(fd,password,31)<=0){
    perror("Can not read file.\n");
    return -1;
  }else{
    password[31]='\0';
    char *nextToken=strsep(&password," \n\r");
    if(strlen(nextToken)<1){
      printf("The password file must start with a password.\n");
      return -1;
    }
    password=nextToken;
    return 0;
  }
}


/* Callback called by the library upon receiving incoming call */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
                             pjsip_rx_data *rdata)
{
  pjsua_call_info ci;

  PJ_UNUSED_ARG(acc_id);
  PJ_UNUSED_ARG(rdata);

  pjsua_call_get_info(call_id, &ci);

  PJ_LOG(3,(THIS_FILE, "Incoming call from %.*s",
                        (int)ci.remote_info.slen,
                        ci.remote_info.ptr));

  /* Automatically answer incoming calls with 200/OK */
  //DAP pjsua_call_answer(call_id, 200, NULL, NULL);

  /* DAP */
  current_call=call_id;
  (void) fflush(stdout);
}


/* Callback called by the library when call's state has changed */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
  pjsua_call_info ci;

  PJ_UNUSED_ARG(e);

  pjsua_call_get_info(call_id, &ci);
  PJ_LOG(3,(THIS_FILE, "Call %d state=%.*s", call_id,
                       (int)ci.state_text.slen,
                       ci.state_text.ptr));

  /* DAP */
  current_call=call_id;
  (void) fflush(stdout);
}


/* Callback called by the library when call's media state has changed */
static void on_call_media_state(pjsua_call_id call_id)
{
  pjsua_call_info ci;

  pjsua_call_get_info(call_id, &ci);

  if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
      // When media is active, connect call to sound device.
      pjsua_conf_connect(ci.conf_slot, 0);
      pjsua_conf_connect(0, ci.conf_slot);
  }

  /* DAP */
  current_call=call_id;
  (void) fflush(stdout);
}


/* Display error and exit application */
static void error_exit(const char *title, pj_status_t status)
{
  pjsua_perror(THIS_FILE, title, status);
  pjsua_destroy();
  exit(1);
}


/* * * * * * * * * * * * * * * * * * *
 * main()                            *
 *                                   *
 * argv[8] may contain URL to call.  *
 * * * * * * * * * * * * * * * * * * */
     
int main(int argc, char *argv[])
{

    /* DAP */
    #define SIPDOMAIN     1
    #define SIPUSER       2
    #define SIPPASS       3
    #define AUDIOCAPTURE  4
    #define AUDIOPLAYBACK 5
    #define SIPDEST       6

    pjsua_acc_id acc_id;
    pj_status_t status;

    /* DAP */
    int udp_port;

    /* Create pjsua first! */
    status = pjsua_create();
    if (status != PJ_SUCCESS) error_exit("Error in pjsua_create()", status);

    /* If argument is specified, it's got to be a valid SIP URL */
    if (argc == (SIPDEST + 1)) {
        status = pjsua_verify_sip_url(argv[SIPDEST]);
        if (status != PJ_SUCCESS) error_exit("Invalid URL in argv", status);
    }


    /* Init pjsua */
    {
        pjsua_config cfg;
        pjsua_logging_config log_cfg;

        pjsua_config_default(&cfg);
        cfg.cb.on_incoming_call = &on_incoming_call;
        cfg.cb.on_call_media_state = &on_call_media_state;
        cfg.cb.on_call_state = &on_call_state;

        pjsua_logging_config_default(&log_cfg);
        log_cfg.console_level = 4;

        status = pjsua_init(&cfg, &log_cfg, NULL);
        if (status != PJ_SUCCESS) error_exit("Error in pjsua_init()", status);
    }


    /* DAP Audio devices */
    pj_caching_pool cp;
    pj_caching_pool_init(&cp, &pj_pool_factory_default_policy, 0);

    status = pjmedia_aud_subsys_init(&cp.factory);
    if (status != PJ_SUCCESS) {
        pj_caching_pool_destroy(&cp);
        pj_shutdown();
        error_exit("pjmedia_aud_subsys_init()", status);
    }
    list_devices();

    
    
    /* Add UDP transport. */
    {
        pjsua_transport_config cfg;
        pjsua_transport_config_default(&cfg);

        /* DAP Support up to 10 simultaneous instances of app */
        for (udp_port=5060; udp_port<5070; udp_port++) {
          cfg.port = udp_port;
          status = pjsua_transport_create(PJSIP_TRANSPORT_UDP, &cfg, NULL);
          if (status == PJ_SUCCESS) break;
          puts("Error creating transport on port 5060-5069, trying next");
        }
        /* DAP */

        // Daniel TCP ADDED 20220331
        status = pjsua_transport_create(PJSIP_TRANSPORT_TCP, &cfg, NULL);

        if (status != PJ_SUCCESS) error_exit("Error creating transport", status);
    }


    /* Initialization is done, now start pjsua */
    status = pjsua_start();
    if (status != PJ_SUCCESS) error_exit("Error starting pjsua", status);


    /* DAP Set Sound Devices from audodemo tool */
//    status = pjsua_set_snd_dev(atoi(argv[AUDIOCAPTURE]),atoi(argv[AUDIOPLAYBACK]));
status = pjsua_set_snd_dev(PJSUA_SND_NULL_DEV,PJSUA_SND_NULL_DEV);
    if (status != PJ_SUCCESS) error_exit("Error in pjsua_set_snd_dev()", status);
    /* DAP */


    /* Register to SIP server by creating SIP account. */
    {
        pjsua_acc_config cfg;
        char uribuf[128];
        char idbuf[128];
        char password[32];

        if (get_password(argv[SIPPASS], password))
          error_exit("Password file processing failed",1);

        pjsua_acc_config_default(&cfg);

        /* DAP */
        sprintf(idbuf,"sip:%s@%s",argv[SIPUSER],argv[SIPDOMAIN]);
        cfg.id = pj_str(idbuf);
        sprintf(uribuf,"sip:%s",argv[SIPDOMAIN]);
        cfg.reg_uri = pj_str(uribuf);
        cfg.cred_count = 1;
        cfg.cred_info[0].realm = pj_str(argv[SIPDOMAIN]);
//cfg.cred_info[0].realm = pj_str("*");
        cfg.cred_info[0].scheme = pj_str("digest");
        cfg.cred_info[0].username = pj_str(argv[SIPUSER]);
        cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
        cfg.cred_info[0].data = pj_str(password);
        /* DAP */

        status = pjsua_acc_add(&cfg, PJ_TRUE, &acc_id);
        if (status != PJ_SUCCESS) error_exit("Error adding account", status);
    }


    /* DAP If URL is specified, make call to the URL. */
    if (argc == (SIPDEST + 1)) {
        pj_str_t uri = pj_str(argv[SIPDEST]);
        status = pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, NULL);
        if (status != PJ_SUCCESS) error_exit("Error making call", status);
    }


    /* Wait until user press "q" to quit. */
    for (;;) {

        /* DAP */
        char option[64];
        /* DAP char option[10]; */

        /* DAP Danel 20220331 add tcp calling */
        PJ_LOG(3,(THIS_FILE, "----------------------------------------------"));
        PJ_LOG(3,(THIS_FILE, "Daniel's VSSP (Very Small SIP phone)"));
        PJ_LOG(3,(THIS_FILE, "----------------------------------------------"));
        PJ_LOG(3,(THIS_FILE, "Enter a To Answer a Call"));
        PJ_LOG(3,(THIS_FILE, "Enter c <PhoneNumber> To place PSTN call using udp (default)"));
        PJ_LOG(3,(THIS_FILE, "Enter C <PhoneNumber> To place PSTN call using tcp"));
        PJ_LOG(3,(THIS_FILE, "Enter h To hangup all calls"));
        PJ_LOG(3,(THIS_FILE, "Enter # <DTMFstring> To dial out RFC2833"));
        PJ_LOG(3,(THIS_FILE, "Enter r <PhoneNumber> To redirect incoming call"));
        PJ_LOG(3,(THIS_FILE, "Enter l list audio devices"));
        PJ_LOG(3,(THIS_FILE, "Enter q To Quit"));
        PJ_LOG(3,(THIS_FILE, "----------------------------------------------"));

        if (fgets(option, sizeof(option), stdin) == NULL) {
            puts("EOF while reading stdin, will quit now..");
            break;
        }
        option[strlen(option)-1]=0x00;

        if (option[0] == 'l') {
            list_devices();
        }

        if (option[0] == 'c') {
          char sipurl[128];
          sprintf(sipurl,"sip:%s@%s",option+2,argv[SIPDOMAIN]);
          status = pjsua_verify_sip_url(sipurl);
          if (status != PJ_SUCCESS) {
            puts("Invalid URL hardcoded");
          } else {
            pj_str_t uri = pj_str(sipurl);
            status = pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, NULL);
            if (status != PJ_SUCCESS)
                PJ_LOG(3,(THIS_FILE, "Error making call"));
          }
        }

        // Daniel 20220331 added tcp calling
        if (option[0] == 'C') {
          char sipurl[128];
          sprintf(sipurl,"sip:%s@%s;transport=tcp",option+2,argv[SIPDOMAIN]);
          status = pjsua_verify_sip_url(sipurl);
          if (status != PJ_SUCCESS) {
            puts("Invalid URL hardcoded");
          } else {
            pj_str_t uri = pj_str(sipurl);
            status = pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, NULL);
            if (status != PJ_SUCCESS)
                PJ_LOG(3,(THIS_FILE, "Error making call"));
          }
        }

        if (option[0] == '#') {
          char buf[128];
          pj_str_t digits;

          if (current_call == -1)
          {
            PJ_LOG(3,(THIS_FILE, "No current call"));
            fflush(stdout);
            continue;
          }
          sprintf(buf,"%s",option+2);
          digits = pj_str(buf);
          status = pjsua_call_dial_dtmf(current_call, &digits);
          if (status != PJ_SUCCESS) {
            pjsua_perror(THIS_FILE, "Unable to send DTMF", status);
          } else {
            PJ_LOG(3,(THIS_FILE, "DTMF digits enqueued for transmission"));
          }
        }

        if (option[0] == 'a') {
          char buf[128];
          pjsua_msg_data msg_data;
          pjsua_call_info call_info;

          if (current_call != -1) {
            pjsua_call_get_info(current_call, &call_info);
          } else {
            /* Make compiler happy */
            call_info.role = PJSIP_ROLE_UAC;
            call_info.state = PJSIP_INV_STATE_DISCONNECTED;
          }
          if (current_call == -1 || 
            call_info.role != PJSIP_ROLE_UAS ||
            call_info.state >= PJSIP_INV_STATE_CONNECTING)
          {
            PJ_LOG(3,(THIS_FILE, "No pending incoming call"));
            fflush(stdout);
            continue;
          }
            else
          {
            int st_code;
            char contact[120];
            pj_str_t hname = { "Contact", 7 };
            pj_str_t hvalue;
            pjsip_generic_string_hdr hcontact;
            //if (!simple_input("Answer with code (100-699)", buf, sizeof(buf)))
            //  continue;
            //st_code = my_atoi(buf);
            st_code=200;
            //if (st_code < 100)
            //  continue;
            pjsua_msg_data_init(&msg_data);
            if (st_code/100 == 2)
            {
              sprintf(contact,"sip:%s@%s",argv[SIPUSER],argv[SIPDOMAIN]);
              //sprintf(contact,"sip:dist_9101@192.168.0.67");
              //if (!simple_input("Enter URL to be put in Contact",contact, sizeof(contact)))
              //  continue;
              hvalue = pj_str(contact);
              pjsip_generic_string_hdr_init2(&hcontact, &hname, &hvalue);
              pj_list_push_back(&msg_data.hdr_list, &hcontact);
            }
            if (current_call == -1) {
              PJ_LOG(3,(THIS_FILE, "Call has been disconnected"));
              fflush(stdout);
              continue;
            }
            pjsua_call_answer(current_call, st_code, NULL, &msg_data);
          }
        }

        if (option[0] == 'r') {
          char buf[128];
          pjsua_msg_data msg_data;
          pjsua_call_info call_info;
          if (current_call != -1) {
            pjsua_call_get_info(current_call, &call_info);
          } else {
            /* Make compiler happy */
            call_info.role = PJSIP_ROLE_UAC;
            call_info.state = PJSIP_INV_STATE_DISCONNECTED;
          }
          if (current_call == -1 ||
            call_info.role != PJSIP_ROLE_UAS ||
            call_info.state >= PJSIP_INV_STATE_CONNECTING)
          {
            PJ_LOG(3,(THIS_FILE, "No pending incoming call"));
            fflush(stdout);
            continue;
          }
            else
          {
            char contact[120];
            pj_str_t hname = { "Contact", 7 };
            pj_str_t hvalue;
            pjsip_generic_string_hdr hcontact;
            pjsua_msg_data_init(&msg_data);
            sprintf(buf,"sip:%s@%s",option+2,argv[SIPDOMAIN]);
            hvalue = pj_str(buf);
            pjsip_generic_string_hdr_init2(&hcontact, &hname, &hvalue);
            pj_list_push_back(&msg_data.hdr_list, &hcontact);
            if (current_call == -1) {
              PJ_LOG(3,(THIS_FILE, "Call has been disconnected"));
              fflush(stdout);
              continue;
            }
            pjsua_call_answer(current_call, 300, NULL, &msg_data);
          }
        }

        /* DAP */

        if (option[0] == 'q')
            break;

        if (option[0] == 'h')
            pjsua_call_hangup_all();

        (void) fflush(stdout);

    }

    /* Destroy pjsua */
    pjsua_destroy();

    return 0;
}

