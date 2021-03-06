/*
 * Amazon FreeRTOS V1.4.7
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://aws.amazon.com/freertos
 * http://www.FreeRTOS.org
 */



/* FreeRTOS includes. */

#include "FreeRTOS.h"













/*-----------------------------------------------------------*/

/**
 * @brief Application runtime entry point.
 */


/*-----------------------------------------------------------*/


/*-----------------------------------------------------------*/



/*-----------------------------------------------------------*/

/*-----------------------------------------------------------*/


/*-----------------------------------------------------------*/

extern void esp_vApplicationTickHook();
void IRAM_ATTR vApplicationTickHook()
{
    esp_vApplicationTickHook();
}

/*-----------------------------------------------------------*/
extern void esp_vApplicationIdleHook();
void vApplicationIdleHook()
{
    esp_vApplicationIdleHook();
}

/*-----------------------------------------------------------*/

void vApplicationDaemonTaskStartupHook( void )
{
}


//================= BLT Code =============================
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_bt_device.h"
#include "esp_spp_api.h"

#include "time.h"
#include "sys/time.h"
#include "jsmn.h"

#define SPP_TAG "SPP_ACCEPTOR_DEMO"
#define SPP_SERVER_NAME "SPP_SERVER"
#define EXCAMPLE_DEVICE_NAME "ESP_SPP_BT"
#define SPP_SHOW_DATA 0
#define SPP_SHOW_SPEED 1
#define SPP_SHOW_MODE SPP_SHOW_DATA    /*Choose show mode: show data or speed*/

static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;

static struct timeval time_new, time_old;
static long data_num = 0;

static const esp_spp_sec_t sec_mask = ESP_SPP_SEC_AUTHENTICATE;
static const esp_spp_role_t role_slave = ESP_SPP_ROLE_SLAVE;
void parse(uint8_t  *json_data, uint16_t json_data_len);

#if (SPP_SHOW_MODE == SPP_SHOW_DATA)
#define SPP_DATA_LEN 20
#else
#define SPP_DATA_LEN ESP_SPP_MAX_MTU
#endif
static uint8_t spp_data[] = "Hello From ESP BT\n";
static void print_speed(void)
{
    float time_old_s = time_old.tv_sec + time_old.tv_usec / 1000000.0;
    float time_new_s = time_new.tv_sec + time_new.tv_usec / 1000000.0;
    float time_interval = time_new_s - time_old_s;
    float speed = data_num * 8 / time_interval / 1000.0;
    ESP_LOGI(SPP_TAG, "speed(%fs ~ %fs): %f kbit/s" , time_old_s, time_new_s, speed);
    data_num = 0;
    time_old.tv_sec = time_new.tv_sec;
    time_old.tv_usec = time_new.tv_usec;
}

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event) {
    case ESP_SPP_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
        esp_bt_dev_set_device_name(EXCAMPLE_DEVICE_NAME);
        esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
        esp_spp_start_srv(sec_mask,role_slave, 0, SPP_SERVER_NAME);
        break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
        break;
    case ESP_SPP_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
        break;
    case ESP_SPP_CLOSE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT");
        break;
    case ESP_SPP_START_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
        break;
    case ESP_SPP_CL_INIT_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
        break;
    case ESP_SPP_DATA_IND_EVT:
#if (SPP_SHOW_MODE == SPP_SHOW_DATA)
        ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len=%d handle=%d",
                 param->data_ind.len, param->data_ind.handle);
        //ESP_LOG_BUFFER_CHAR("",param->data_ind.data,param->data_ind.len);
        parse(param->data_ind.data, param->data_ind.len);
        ESP_LOGI(SPP_TAG, "Succedded to parse JSON: \n");
        esp_spp_write(param->srv_open.handle, sizeof(spp_data) - 1, spp_data);
#else
        gettimeofday(&time_new, NULL);
        data_num += param->data_ind.len;
        if (time_new.tv_sec - time_old.tv_sec >= 3) {
            print_speed();
        }
#endif
        break;
    case ESP_SPP_CONG_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
        break;
    case ESP_SPP_WRITE_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
        break;
    case ESP_SPP_SRV_OPEN_EVT:
        ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT");
        gettimeofday(&time_old, NULL);
        break;
    default:
        break;
    }
}

void esp_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS) {
            ESP_LOGI(SPP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(SPP_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        } else {
            ESP_LOGE(SPP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }
    case ESP_BT_GAP_PIN_REQ_EVT:{
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_PIN_REQ_EVT min_16_digit:%d", param->pin_req.min_16_digit);
        if (param->pin_req.min_16_digit) {
            ESP_LOGI(SPP_TAG, "Input pin code: 0000 0000 0000 0000");
            esp_bt_pin_code_t pin_code = {0};
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 16, pin_code);
        } else {
            ESP_LOGI(SPP_TAG, "Input pin code: 1234");
            esp_bt_pin_code_t pin_code;
            pin_code[0] = '1';
            pin_code[1] = '2';
            pin_code[2] = '3';
            pin_code[3] = '4';
            esp_bt_gap_pin_reply(param->pin_req.bda, true, 4, pin_code);
        }
        break;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(SPP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif

    default: {
        ESP_LOGI(SPP_TAG, "event: %d", event);
        break;
    }
    }
    return;
}
    jsmntok_t t[128]; /* We expect no more than 128 JSON tokens */
jsmn_parser p;
void app_main()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_init()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bluedroid_enable()) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_bt_gap_register_callback(esp_bt_gap_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s gap register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp register failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

    if ((ret = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE(SPP_TAG, "%s spp init failed: %s\n", __func__, esp_err_to_name(ret));
        return;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

    /*
     * Set default parameters for Legacy Pairing
     * Use variable pin, input pin code when pairing
     */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_VARIABLE;
    esp_bt_pin_code_t pin_code;
    esp_bt_gap_set_pin(pin_type, 0, pin_code);
}




static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}
uint32_t motor_data[10];
char buffer[50]; 
typedef enum
{
    SH  = 0,
    SV  = 1,
    STP = 2,
    M1  = 3,
    M2  = 4,
    M3  = 5
}Data_Json_Token_t;
void parse(uint8_t  *json_data, uint16_t json_data_len)
{
    
    int i;
    int r;
    jsmn_init(&p);
    r = jsmn_parse(&p, (const char *)json_data, json_data_len, t, 128);
    ESP_LOGI("myTag", "Tokens nuber %d\n", r);
    if (r > 0) 
    {
        ESP_LOGI("myTag", "Tokens nuber %d\n", r);
       for (i = 1; i < r; i++) 
       {    ESP_LOGI("myTagyyy","- User: %.*s\n", t[i].end - t[i].start,
                    (const char *)json_data+ t[i].start);
            if (jsoneq((const char *)json_data, &t[i], "sh") == 0) 
            {
                /* We may use strndup() to fetch string value */
                ESP_LOGI("myTag","- sh: %.*s\n", t[i + 1].end - t[i + 1].start,
                    (const char *)json_data+ t[i + 1].start);
                sprintf(buffer, "%.*s", t[i + 1].end - t[i + 1].start,
                    (const char *)json_data+ t[i + 1].start);
                ESP_LOGI("Integer", " value : %d", atoi(buffer));
                motor_data[SH] = atoi(buffer);
                i++;
            } 
            else if (jsoneq((const char *)json_data, &t[i], "sv") == 0)
            {
                /* We may additionally check if the value is either "true" or "false" */
                ESP_LOGI("myTag","- sv: %.*s\n", t[i + 1].end - t[i + 1].start,
                        (const char *)json_data+ t[i + 1].start);
                sprintf(buffer, "%.*s", t[i + 1].end - t[i + 1].start,
                    (const char *)json_data+ t[i + 1].start);
                ESP_LOGI("Integer", " value : %d", atoi(buffer));
                motor_data[SV] = atoi(buffer);
                i++;
            } 
            else if (jsoneq((const char *)json_data, &t[i], "stp") == 0)
            {
                /* We may want to do strtol() here to get numeric value */
                ESP_LOGI("myTag","- stp: %.*s\n", t[i + 1].end - t[i + 1].start,
                 (const char *)json_data+ t[i + 1].start);
                sprintf(buffer, "%.*s", t[i + 1].end - t[i + 1].start,
                    (const char *)json_data+ t[i + 1].start);
                ESP_LOGI("Integer", " value : %d", atoi(buffer));
                motor_data[STP] = atoi(buffer);
                i++;
            }
            else if (jsoneq((const char *)json_data, &t[i], "m1") == 0)
            {
                /* We may want to do strtol() here to get numeric value */
                ESP_LOGI("myTag","- m1: %.*s\n", t[i + 1].end - t[i + 1].start,
                 (const char *)json_data+ t[i + 1].start);
                sprintf(buffer, "%.*s", t[i + 1].end - t[i + 1].start,
                    (const char *)json_data+ t[i + 1].start);
                ESP_LOGI("Integer", " value : %d", atoi(buffer));
                motor_data[M1] = atoi(buffer);
                i++;
            } 
            else if (jsoneq((const char *)json_data, &t[i], "m2") == 0)
            {
                /* We may want to do strtol() here to get numeric value */
                ESP_LOGI("myTag","- m2: %.*s\n", t[i + 1].end - t[i + 1].start,
                 (const char *)json_data+ t[i + 1].start);
                sprintf(buffer, "%.*s", t[i + 1].end - t[i + 1].start,
                    (const char *)json_data+ t[i + 1].start);
                ESP_LOGI("Integer", " value : %d", atoi(buffer));
                motor_data[M2] = atoi(buffer);
                i++;
            }
            else if (jsoneq((const char *)json_data, &t[i], "m3") == 0)
            {
                /* We may want to do strtol() here to get numeric value */
                ESP_LOGI("myTag","- m3: %.*s\n", t[i + 1].end - t[i + 1].start,
                 (const char *)json_data+ t[i + 1].start);
                sprintf(buffer, "%.*s", t[i + 1].end - t[i + 1].start,
                    (const char *)json_data+ t[i + 1].start);
                ESP_LOGI("Integer", " value : %d", atoi(buffer));
                motor_data[M3] = atoi(buffer);
                i++;
            }
            else
            {
                ESP_LOGI("myTag","Unexpected key: %.*s\n", t[i].end - t[i].start,
                 (const char *)json_data+ t[i].start);
            }
        } 
    }

    for(i = 0; i < 6; i++)
    {
        ESP_LOGI("Data", "Value %d = %d", i, motor_data[i]);
    }
    
}