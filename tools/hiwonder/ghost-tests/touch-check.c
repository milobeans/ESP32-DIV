#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "lvgl_touch/ft6x36.h"
#include "esp_err.h"
static uint8_t packet[5];
static int fail_read;
void vTaskDelay(unsigned ticks){assert(ticks==120);}
const char *esp_err_to_name(int err){(void)err;return "fake";}
esp_err_t lvgl_i2c_read(int port,uint16_t addr,uint32_t reg,uint8_t *out,uint16_t size){
 assert(port==1 && addr==0x38);
 if(size==5){assert(reg==2);if(fail_read)return ESP_FAIL;memcpy(out,packet,5);}
 else{assert(size==1);*out=reg==0xa8?0x64:0;}
 return ESP_OK;
}
static lv_indev_data_t sample(unsigned x,unsigned y,unsigned contacts,unsigned event){
 packet[0]=contacts;packet[1]=(event<<6)|((x>>8)&15);packet[2]=x;packet[3]=(y>>8)&15;packet[4]=y;
 lv_indev_data_t out={0};assert(!ft6x36_read(NULL,&out));return out;
}
int main(void){
 ft6x06_init(0x38);
 lv_indev_data_t d=sample(0,0,1,0);assert(d.state==1 && d.point.x==0 && d.point.y==239);
 d=sample(239,319,1,2);assert(d.state==1 && d.point.x==319 && d.point.y==0);
 d=sample(123,234,2,2);assert(d.state==1 && d.point.x==234 && d.point.y==116);
 d=sample(240,319,1,0);assert(d.state==0);
 d=sample(0,320,1,0);assert(d.state==0);
 d=sample(0,0,0,0);assert(d.state==0);
 d=sample(0,0,3,0);assert(d.state==0);
 d=sample(0,0,1,1);assert(d.state==0);
 d=sample(0,0,1,3);assert(d.state==0);
 fail_read=1;d=sample(0,0,1,0);assert(d.state==0);
 puts("Hiwonder touch coordinates and invalid-read check passed");
}
