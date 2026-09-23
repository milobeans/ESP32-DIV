#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "hiwonder_board.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
static unsigned char regs[8];
static int fail_reg=-1, boot_level=1, write_count, lock_available=1, mutex_created;
const char *esp_err_to_name(int err) {(void)err;return "fake";}
esp_err_t gpio_config(const gpio_config_t *cfg) {assert(cfg->pin_bit_mask==1);assert(cfg->mode==GPIO_MODE_INPUT);return ESP_OK;}
int gpio_get_level(int pin) {assert(pin==0);return boot_level;}
SemaphoreHandle_t xSemaphoreCreateMutex(void) {mutex_created++;return &mutex_created;}
int xSemaphoreTake(SemaphoreHandle_t sem,unsigned timeout) {assert(sem);assert(timeout<=50);return lock_available;}
int xSemaphoreGive(SemaphoreHandle_t sem) {assert(sem);return 1;}
esp_err_t lvgl_i2c_init(int port) {assert(port==0);return ESP_OK;}
esp_err_t lvgl_i2c_read(int port,uint16_t addr,uint32_t reg,uint8_t *out,uint16_t size) {assert(port==0 && addr==0x20 && reg<8 && size==1);if(reg==(unsigned)fail_reg)return ESP_FAIL;*out=regs[reg];return ESP_OK;}
esp_err_t lvgl_i2c_write(int port,uint16_t addr,uint32_t reg,const uint8_t *data,uint16_t size) {assert(port==0 && addr==0x20 && reg<8 && size==1);assert(reg==3 || reg==4 || reg==6 || reg==7);regs[reg]=*data;write_count++;return ESP_OK;}
int main(int argc,char **argv) {
  assert(argc==2);memset(regs,0xff,sizeof regs);regs[4]=0x55;regs[6]=0x0f;
  if(strcmp(argv[1],"failure")==0){
    fail_reg=3;assert(hiwonder_board_init()!=ESP_OK);assert(regs[3]==0xff && regs[7]==0xff);
    int writes=write_count;assert(hiwonder_board_set_backlight(true)!=ESP_OK);assert(write_count==writes);
    boot_level=0;assert(hiwonder_board_read_keys()==0x10);
  }else{
    assert(hiwonder_board_init()==ESP_OK);assert(regs[4]==0x05 && regs[6]==0xff && regs[3]==0xbf && regs[7]==0xbf);
    assert(regs[1]==0xff && regs[2]==0xff && regs[5]==0xff);
    assert(hiwonder_board_set_backlight(true)==ESP_OK && regs[3]==0xff);
    int writes=write_count;assert(hiwonder_board_set_backlight(true)==ESP_OK && write_count==writes);
    assert(hiwonder_board_set_backlight(false)==ESP_OK && regs[3]==0xbf);
    regs[0]=0x0f;assert(hiwonder_board_read_keys()==0x0f);boot_level=0;assert(hiwonder_board_read_keys()==0x1f);
    regs[0]=0xff;assert(hiwonder_board_read_keys()==0x10);fail_reg=0;assert(hiwonder_board_read_keys()==0x10);
    lock_available=0;assert(hiwonder_board_set_backlight(true)==ESP_ERR_TIMEOUT && regs[3]==0xbf);
    assert(hiwonder_board_read_keys()==0x10);assert(hiwonder_board_init()==ESP_OK && mutex_created==1);
  }
  puts("Hiwonder expander check passed");return 0;
}
