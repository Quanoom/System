### DATE & TIME SYSTEM : 
This system allows users to view the current time and date in either English or French.
Users can select their preferred language, and the system will display the time and date formatted accordingly. The system uses a real time clock (RTC).
The language selection is done by a push button, the validation is done by a push button.

#### COMPONENTS : 
- PIC18F452
- REAL TIME CLOCK RTC MAX6902(SPI protocol)
- PUSH BUTTON * 2.

#### Notice:
In order to set the current date & time you have to change the array (bytes in the definitions.h) to the current date & time.

#### Contributions : 
Contributions are welcomed, you can add new features like : 
- New languages.
- Calendar.
- Generating alarm.

##### CHECK : 
- [@show](https://github.com/0xaB26/realTimeClockSystem/blob/main/show.mp4)
