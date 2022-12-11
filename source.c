#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SIZE_OF_BUFFER 5
void GET_BUTTON_STATE(char* path, int* button_pid); //Прототип функции чтения кнопки
struct timespec mt1; // Сохраняем время здесь

// Чтение значения в кнопку
void GET_BUTTON_STATE(char* path, int* button_pid)
{ 
    FILE *button_file;
    button_file = fopen(path, "r");
    fscanf(button_file, "%d", button_pid);;
    fclose(button_file);
}


int main()
{
    
    int counter = 0;
    int hours;
    int minutes;
    int seconds;
    int config_hours = 0;
    int config_seconds = 0;
    int config_minutes = 0;

    int BTTN1_State ;
    int BTTN2_State;
    int SW_State;
    int TOTAL_TIME = 0; // Текущее время (изначально 00:00:00)
    int TIME_SETTING = 0; // Режим настройки времени
    int BASE_TIME = 0;
    int BASE_ENCODER_VALUE; // for config

    int BTTN1_previous_state;
    int BTTN2_previous_state;
    int SW_previous_state;
    int select_unit = 0;

    char value[100];
    char output_time[100];

    char *myFifo = "data_path";
    char *myFifo2 = "data_out";
    mkfifo(myFifo, 0666);
    mkfifo(myFifo2, 0666);
    
    int fd = open(myFifo, O_RDONLY | O_NONBLOCK);
    int fdB = open(myFifo2, O_WRONLY);
    
    while(1) {       
        read(fd, value, 100);
        sleep(0.01);
        counter += 1;    
        if (counter == 10000) {
            switch( TIME_SETTING )
            {          
                case 0: {                        
                    hours = (TOTAL_TIME) % 86400 / 3600;
                    minutes = (TOTAL_TIME%3600)/60;
                    seconds = (TOTAL_TIME%60); 
                    TOTAL_TIME += 1;

                    printf("%d:%d:%d\n", hours, minutes, seconds);           
                    printf("Encoder value: %d\n", atoi(value)/18);

                    // Вывод значения с кнопок:
                    GET_BUTTON_STATE("/sys/class/gpio/gpio23/value", &BTTN1_State); // Сдвиг влево
                    GET_BUTTON_STATE("/sys/class/gpio/gpio22/value", &BTTN2_State); // Сдвиг вправо 
                    GET_BUTTON_STATE("/sys/class/gpio/gpio26/value", &SW_State); // Перейти в режим настройки             
                    
                    printf("Button 1 state: %d\n", BTTN1_State);
                    printf("Button 2 state: %d\n", BTTN2_State);
                    printf("SW state: %d\n", SW_State);  
                    
                    // Склеиваем строку
                    sprintf(output_time, "%d:%d:%d\0", hours, minutes, seconds);   

                    write(fdB, output_time, strlen(output_time));
                    write(fdB, "\n", 1);
    
                    counter = 0;
                    
                    if (SW_State == 0 && SW_previous_state == 1){ 
                        TIME_SETTING = 1;
                    }                     
              

                    SW_previous_state = SW_State; 
                    BTTN1_previous_state = BTTN1_State;
                    BTTN2_previous_state = BTTN2_State;
                    break;
                }
                case 1: {
                    printf("In time setting\n");
                    GET_BUTTON_STATE("/sys/class/gpio/gpio23/value", &BTTN1_State); // Сдвиг влево
                    GET_BUTTON_STATE("/sys/class/gpio/gpio22/value", &BTTN2_State); // Сдвиг вправо 
                    GET_BUTTON_STATE("/sys/class/gpio/gpio26/value", &SW_State); // Перейти в режим настройки                   

                    if (SW_State == 0 && SW_previous_state == 1){ 
                        // Выходим из функции, изменяя время
                        printf("Время изменено на %d:%d:%d\n", config_hours, config_minutes, config_seconds);
                        // Пересчитаем total time исходя из новых параметров
                        TOTAL_TIME = config_hours*3600 + config_minutes*60 + config_seconds;
                        config_hours = 0;
                        config_minutes = 0;
                        config_seconds = 0; 
                        TIME_SETTING = 0;
                    }  
                    counter = 0;

                    if (BTTN1_State == 0 && BTTN1_previous_state == 1) {
                        if (select_unit != 3) select_unit += 1;
                        
                        minutes = config_minutes;
                        seconds = config_seconds;
                        hours = config_hours;
                        
                        
                        BASE_ENCODER_VALUE = atoi(value)/18; // Фиксируем на приращении энкодера                                                        
                    }

                    else if (BTTN2_State == 0 && BTTN2_previous_state == 1) {
                        if (select_unit != 0) select_unit -= 1;
                        BASE_ENCODER_VALUE = atoi(value)/18;

                        minutes = config_minutes;
                        seconds = config_seconds;
                        hours = config_hours;
                        
                    }
                    
                    switch (select_unit)
                    {
                        case 1:
                            // Регулировка часов
                            config_hours = (hours + atoi(value)/18 - BASE_ENCODER_VALUE) % 24;
                            break;
                        case 2:
                            // Регулировка минут
                            config_minutes = (minutes + atoi(value)/18 - BASE_ENCODER_VALUE) % 60;
                            break;
                        case 3:
                            // Регулировка секунд
                            config_seconds = (seconds + atoi(value)/18 - BASE_ENCODER_VALUE) % 60;
                            break;
                        default:
                            config_hours = config_hours;
                            break;
                    }
                  
                    printf("%d:%d:%d\n", config_hours, config_minutes, config_seconds);
                    printf("Encoder value: %d\n", atoi(value)/18);
                    printf("Select unit: %d\n", select_unit);
                    SW_previous_state = SW_State; 
                    BTTN1_previous_state = BTTN1_State;
                    BTTN2_previous_state = BTTN2_State;
                    
                    break;
                }
            }
        }
    }
    close(fd);
    return 0;
}