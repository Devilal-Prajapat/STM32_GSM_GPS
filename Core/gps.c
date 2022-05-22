#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "gps.h"

//char gga[] = "$GPGGA,181908.00,3404.7041778,N,07044.3966270,W,4,13,1.00,495.144,M,29.200,M,0.10,0000*40";
//char rmc[] = "$GPRMC,144326.00,A,5107.0017737,N,11402.3291611,W,0.080,323.3,210307,0.0,E,A*20";

enum
{
  LATITUDE,
  LATITUDE_DIR,
  LONGITUDE,
  LONGITUDE_DIR,
  TIME,
  GPS_VALID,
  ALTITUDE,
  ALTITUDE_UNIT,
};

char valid_time1[9];
char valid_date[6];
char valid_lat[10];
char valid_lng[11];
char valid_lat_dir;
char valid_lng_dir;
char valid_alt_unit;
char valid_gps_fix = 0;
char valid_satellite[2];
char valid_altitude[6];
char valid_gps_valid;
char valid_speed[5];

char time1[9];
char date[6];
char lat[10];
char lng[11];
char lat_dir;
char lng_dir;
char alt_unit;
char gps_fix = 0;
char satellite[2];
char altitude[6];
char alt_unit;
char gps_valid;
char speed[5];
float latitude = 0.0f;
float longitude = 0.0f ;
uint8_t is_loc_valid;

static void parse_gga(char *ptr);
static void parse_rmc(char *ptr);
static float nmea_to_dd(const char *nmea, char quadrant);
static uint8_t check_char(uint8_t ch);
uint8_t is_valid_data(char *ptr, uint8_t data_type);

void process_gps_data(char *pdata)
{  
  char *buff = (char *)pdata;
  char nmea_buff[100];
  uint16_t len = 0;
  char *ret = strstr(buff,"GGA");  
  char *ret1 = strstr(buff,"RMC");
  
  if(strncmp(ret,"GGA",3) == 0)
  {
    char *astrick = strstr(ret, "*");
    len = astrick - ret;
    if(len < 86)
    {
      strncpy(nmea_buff, ret,len);
      nmea_buff[len]= '\0';
      parse_gga(nmea_buff);
    }
  }     

  if(strncmp(ret1, "RMC", 3) == 0)
  {
    char *astrick1 = strstr(ret1,"*");
    len = astrick1 - ret1;
    if(len < 86)
    {
      strncpy(nmea_buff, ret1,len);
      nmea_buff[len]= '\0';
      parse_rmc(nmea_buff);
    }
  }
  
  if(gps_valid == 'A')
  {
    latitude = nmea_to_dd(lat,lat_dir);
    longitude = nmea_to_dd(lng,lng_dir);  
    is_loc_valid = 1;
    gps_valid = 'V';
  }
}


void parse_gga(char *ptr)
{    
  char *pchar;
  char *str;//(char *)ptr;
  int len  = strlen(ptr);
  strcpy(str,ptr); // if ptr where it originally declared is pointer than use this
  //str = ptr;   // use this if ptr is array
  pchar = str;

  int comma_count= 0;
  while(len)
  {
      len--;
      if(*str == ',')
      {
          comma_count++;
          *str = '\0';
//          if(pchar == str)
//          {
//            pchar = str + 1;
//            continue;
//          }
        
          switch(comma_count)
          {
              case 7: // fix
                gps_fix = pchar[0];
                printf("gps fix %c\n",gps_fix);
              
              break;
              
              case 8: // num of satellite
                strcpy(satellite, pchar);  
                printf("satellite %s\n", satellite);
              break;
              
              case 10: //altitude
                strcpy(altitude, pchar);
                printf("altitude %s", altitude);                
              break;
              
              case 11: //altitude unit
                alt_unit = pchar[0];
                printf("%c\n", alt_unit);
              break;
              
              default:
              break;
          }
          pchar = str+1;
        }
      str++;
    }
}

void parse_rmc(char *ptr)
{
  char *pchar;
  char *str;//(char *)ptr;
  int len  = strlen(ptr);
  strcpy(str,ptr); // if ptr where it originally declared is pointer than use this
  //str = ptr;   // use this if ptr is array
  pchar = str;
  
  int comma_count= 0;
  while(len)
  {
    len--;
    if(*str == ',')
    {
      comma_count++;
      *str = '\0';
//      if(pchar == str)
//      {
//        pchar = str + 1;            
//        continue;
//      }

      switch(comma_count)
      {
        case 1:// gprmc
        break;

        case 2:// time          
          if(is_valid_data(pchar, TIME))
          {
            strncpy(time1, pchar,6);
            time1[6] = '\0';
            printf("time1 %s\n",time1);
          }
        break;
          
        case 3: // gps valid
          if(is_valid_data(pchar, GPS_VALID))
          {
            gps_valid = pchar[0];
            printf("gps_valid %c\n", gps_valid);
          }
        break;

        case 4:// lat
          if(is_valid_data(pchar, LATITUDE))
          {
            strcpy(lat, pchar);
            printf("lat %s",lat);
          }
        break;

        case 5: // lat  dir
          if(is_valid_data(pchar, LATITUDE_DIR))
          {
            lat_dir = pchar[0];
            printf("%c\n",lat_dir);
          }
        break;

        case 6:// lng
          if(is_valid_data(pchar, LONGITUDE))
          {
            strcpy(lng, pchar);
            printf("lng %s",lng);
          }
        break;

        case 7: // lng dir
          if(is_valid_data(pchar, LONGITUDE_DIR))
          {
            lng_dir = pchar[0];
            printf("%c\n",lng_dir);
          }
        break;

        case 8: // speed
          strcpy(speed, pchar);
          printf("speed %s\n", speed);
        break;

        case 9: //track true
          //strcpy(satellite, pchar);
          printf("track %s\n",pchar);
        break;

        case 10: //date
          strcpy(date, pchar);
          printf("date %s\n",date);
        break;

        default:
        break;
      }
      pchar = str+1;
    }
    str++;
  }
}

uint8_t check_char(uint8_t ch)
{
  if(ch >= 0x30 && ch < 0x39)
  {
    return 1;
  }
  return 0;
}

uint8_t is_valid_data(char *ptr, uint8_t data_type)
{
  uint8_t valid = 0;
  switch(data_type)
  {
    case LATITUDE:
      if(ptr[4] == '.')
      {
        valid = 1;
      }
      break;
      
    case LATITUDE_DIR:
      if(ptr[0] == 'N' || ptr[0] == 'S')
      {
        valid = 1;
      }
      break;
      
    case LONGITUDE:
      if(ptr[5] == '.')
      {
        valid = 1;
      }
      break;
      
    case LONGITUDE_DIR:
      if(ptr[0] == 'E' || ptr[0] == 'W')
      {
        valid = 1;
      }
      break;  
      
    case TIME:
      if(ptr[6] == '.')
      {
        valid = 1;
      }
      break;
      
    case GPS_VALID:
      if(ptr[0] == 'A' || ptr[0] == 'V')
      {
        valid = 1;
      }
      break;
      
    default:
      break;
  }
  return valid;
}

float nmea_to_dd(const char *nmea, char quadrant)
{
  double raw_nmea = atof(nmea);
  int dd = (int)(raw_nmea/100);
  double mm = (raw_nmea - dd*100)/60;
  double pos = (dd + mm);
  if(quadrant == 'S' || quadrant == 'W')
  {
    pos = -pos;
  }
  return (float)pos;
}

#if 0
int main()
{
    //printf("Hello World\n");
    parse_gga(gga);
    parse_rmc(rmc);
    return 0;
}
#endif
