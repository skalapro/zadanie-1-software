#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include "driver/uart.h"
#include <stdbool.h>
#include <stdlib.h>

void commands(void);
void action(int n, nvs_handle_t x);
int command_assignment(const char *text);
void cli(void);
void data_assignment(void);
void default_values(nvs_handle_t x);
void command_display(nvs_handle_t x);

char data[4][14] = {
    "time",
    "velocity",
    "engine_check",
    "rocket_name",
}; // tablica z dostepnymi zmiennymi
char type[4][7] = {
    "int",
    "float",
    "bool",
    "string"
}; // tablica z typami dostepnymi
const char avilable_commands[6][11] = {
    "help",
    "set",
    "check",
    "delete",
    "check_all",
    "exit"
}; // tablica zawierajaca dostepne komendy
char current_type[20];
char key[30];
char user[50];
int len = 0;
int turnof = 0;
int32_t odczyt = 0;


void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err != ESP_OK)
    {
        printf("flash memory error\r\n");
        return;
    }
    nvs_handle_t handle;
    ESP_ERROR_CHECK(
    nvs_open("dane",
             NVS_READWRITE,
             &handle)
    );
    default_values(handle);
    uart_driver_install(
        UART_NUM_0,
        1024,
        1024,
        0,
        NULL,
        0
    );

    vTaskDelay(pdMS_TO_TICKS(1000));

    printf("use help to display available commands\r\n");
        
    while(turnof == 0)
    {
        uart_write_bytes(UART_NUM_0, "ESP32> ", strlen("ESP32> "));
        cli();

        int used_command = command_assignment(user);

        action(used_command, handle);
    
    }
    nvs_commit(handle);
    nvs_close(handle);

    printf("Koniec\r\n");

}



void commands(void) // wyslanie spisu komend
{
    for(int i = 1; i < 6; i++)
    {
        printf("%s\r\n", avilable_commands[i]);
    }
}
void action(int n, nvs_handle_t x) // dzialanie zaleznie od wybranej komendy
{
    switch(n)
    {
        case 1: // help
            commands();
            break;

        case 2: // set

            uart_write_bytes(UART_NUM_0, "Variable name: ", strlen("Variable name: "));
            
            cli();

            data_assignment();
                        
            
            if (strcmp(current_type,"none") != 0)
            {   
                uart_write_bytes(UART_NUM_0, "Value: ", strlen("Value: "));                
                if(strcmp(current_type,"int") == 0 || strcmp(current_type, "float") == 0)
                {
                    cli();
                    char *end;
                    float value = strtof(user, &end);
                    if(*end != '\0')
                    {
                        printf("wrong value\r\n");
                    }
                    else
                    {
                        if (strcmp(current_type, "float") == 0)
                        {
                            nvs_set_float(x, key, value);
                        }
                        else
                            if (value != (int32_t)value) 
                                printf("wrong value\r\n");
                            else
                                nvs_set_i32(x, key, value);
                    }
                }
                else if (strcmp(current_type,"bool") == 0)
                {
                    cli();
                    if (strcmp(user,"true") == 0 || strcmp(user,"false") == 0) // obsluga niepoprawnej zmiennej
                    {   
                        if(strcmp(user, "true") == 0) //"zasymulowanie boola"
                            odczyt = 1;
                        else
                            odczyt = 0; 
                        nvs_set_u8(x, key, odczyt);

                    }
                    else 
                        printf("wrong value\r\n"); 
                }
                else if (strcmp(current_type, "string") == 0)
                {
                    cli();
                    nvs_set_str(x, key, user);
                }
                nvs_commit(x);
                break;
            }
            else
            { 
                printf("variable not found\r\n"); 
                break;
            }
            break;         
            
        case 3: // check

            uart_write_bytes(UART_NUM_0, "Variable: ", strlen("Variable: "));

            cli(); // odczyt od uzytkownika
            data_assignment();
            char buf[32];
            int32_t outputi = 0;
            float outputf = 0.0;
            uint8_t temp = 0; // inaczej nvs_get nie dziala z boolem
            bool outputb = 0;
            if(strcmp(current_type,"int") == 0)
            {
                nvs_get_i32(x, key, &outputi);
                snprintf(buf, sizeof(buf), "%ld", outputi);
            }
            else if (strcmp(current_type,"float") == 0)
            {
                nvs_get_float(x, key, &outputf);                
                snprintf(buf, sizeof(buf), "%.3f", outputf); 
            } 
            else if (strcmp(current_type,"bool") == 0)
            {
                nvs_get_u8(x, key, &temp);
                outputb = (bool)temp;
                snprintf(buf, sizeof(buf), "%d", outputb);
            }
            else if (strcmp(current_type, "string") == 0)
            {
                size_t sizet = sizeof(user);
                nvs_get_str(x, key, buf, &sizet);
            }
            else if(strcmp(current_type, "none") == 0)
            {
                printf("variable not found\r\n");
                break;
            }
            printf("%s: %s\r\ndata type: %s\r\n", key, buf, current_type);
            break;
        case 4: // delete
            uart_write_bytes(UART_NUM_0, "Variable: ", strlen("Variable: "));
            cli();
            data_assignment();
            if (strcmp(current_type, "none") != 0)
            {
                nvs_erase_key(x,key);
                nvs_commit(x);
                printf("Deleted\r\n");
            }
            else
            {
                printf("Variable not found\r\n");
            }
            break;
        case 5: // check all
            command_display(x);
            break;
        case 6: // exit
            turnof = 1;
            break;
        default: // obsluga niepoprawnych komend
            printf("error - use valid command, check them by typing help\r\n"); 
            break;

    }
}

int command_assignment(const char *text) // przypisanie komendzie liczby zeby latwiej pozniej tym operowac
{
    int output = 7;
    for(int i = 0; i < 6; i++)
    {
        if(strcmp(text, avilable_commands[i]) == 0)
        {
            output = i + 1; // +1 zeby indeksowanie bylo wygodniejsze (help to 1 komenda)
            break;
        }
    }
    return output;
}
// odczyt pojedynczych lini tekstu w uart z obsluga entera i backspace
void cli(void)
{

    len = 0;
    memset(user, 0, sizeof(user));
    while(1) // odczyt w cli
    {
        char c;
        uart_read_bytes(UART_NUM_0, (uint8_t*)&c, 1, portMAX_DELAY);
        if(c == '\r' || c == '\n') // obsluga entera
        {
            if (len == 0)
                continue;
            
            user[len] = '\0';
            break;    
        }
        else if(c == '\b' || c == 127) // obsluga backspace
        {
            if(len > 0)
            {
                len--;
                
                user[len] = '\0';
                
                uart_write_bytes(UART_NUM_0, "\b \b", 3);
            }
            else
                continue;
        }
        else
        {
            uart_write_bytes(UART_NUM_0, &c, 1);
            user[len] = c;
            
            len++;

        }
    }
    uart_write_bytes(UART_NUM_0, "\r\n", 2);
}
// przypisanie nazwy zmiennej do jej typu i klucza nvs
void data_assignment(void)

{
    int found = -1;
    for (int i = 0; i < 4; i++)
    {
        if (strcmp(user,data[i]) == 0)
        {
            strcpy(key,data[i]);
            strcpy(current_type,type[i]);
            found = 1;
            break;
        }
    }
    if (found == -1) // obsluga blednej zmiennej
    {
        strcpy(key,"none");
        strcpy(current_type,"none");
    }
}
// ustawianie wartosci domyslnych gdy brak ustawien
void default_values(nvs_handle_t x)
{
    int32_t time;

    if(nvs_get_i32(x,
                   "time",
                   &time) == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_i32(x,
                    "time",
                    10); // poczatkowa wartosc dla czasu 10s
    }
    float velocity;
    if(nvs_get_float(x, 
                    "velocity", 
                    &velocity) == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_float(x, 
            "velocity", 
            200.321); // poczatkowa wartosc dla predkosci 200.321km/h
    }

    uint8_t engine;

    if(nvs_get_u8(x,
                  "engine_check",
                  &engine) == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_u8(x,
                   "engine_check",
                   false); // check engine poczatkowe false
    }
    char buf[50];
    size_t sizet = sizeof(buf);
    if(nvs_get_str(x, "rocket_name", 
                    buf, 
                    &sizet) == ESP_ERR_NVS_NOT_FOUND)
    {
        nvs_set_str(x,
                    "rocket_name",
                    "none"); // poczatkowa nazwa dla rakiety, brak
    }
    nvs_commit(x);
}
// wyswietlanie wszystkich zmiennych wraz z typem i wartoscia
void command_display(nvs_handle_t x)
{
    char buf[64];

    for(int i = 0; i < 4; i++)
    {
        if(strcmp(type[i], "int") == 0)
        {
            int32_t val;
            nvs_get_i32(x, data[i], &val);

            snprintf(buf, sizeof(buf), "%ld", val);
        }
        else if(strcmp(type[i], "float") == 0)
        {
            float val;
            nvs_get_float(x, data[i], &val);

            snprintf(buf, sizeof(buf), "%.2f", val);
        }
        else if(strcmp(type[i], "bool") == 0)
        {
            uint8_t val;
            nvs_get_u8(x, data[i], &val);

            snprintf(buf, sizeof(buf), "%s", val ? "true" : "false");
        }
        else if(strcmp(type[i], "string") == 0)
        {
            size_t len = sizeof(buf);
            nvs_get_str(x, data[i], buf, &len);
        }
        printf("%s: %s (%s)\r\n",
            data[i],
            buf,
            type[i]);
    }
}