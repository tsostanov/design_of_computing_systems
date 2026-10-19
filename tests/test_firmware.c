#include "ring_buffer.h"
#include "morse.h"
#include "morse_codec.h"
#include "button.h"
#include "uart_driver.h"
#include "app.h"
#include "fake_hal.h"
#include "usart.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_ring(void)
{
    uint8_t storage[7], data[7] = {0,1,2,3,4,5,6}, v;
    RingBuffer r;
    Ring_Init(&r, storage, 7);
    assert(!Ring_Read(&r, &v));
    for (unsigned cycle = 0; cycle < 1000; ++cycle) {
        assert(Ring_Write(&r, data, 7));
        assert(!Ring_Write(&r, data, 1));
        for (unsigned i = 0; i < 7; ++i) {
            assert(Ring_Peek(&r, &v) && v == i);
            assert(Ring_Read(&r, &v) && v == i);
        }
        assert(Ring_Write(&r, data, 3));
        assert(!Ring_Write(&r, data, 5)); /* all-or-nothing */
        for (unsigned i = 0; i < 3; ++i) assert(Ring_Read(&r, &v) && v == i);
    }
}
static void test_codec(void)
{
    const char *expected[] = {".-","-...","-.-.","-..",".","..-.","--.","....","..",
        ".---","-.-",".-..","--","-.","---",".--.","--.-",".-.","...","-","..-","...-",
        ".--","-..-","-.--","--.."};
    for (unsigned i = 0; i < 26; ++i) {
        assert(strcmp(Morse_Encode((char)('A'+i)), expected[i]) == 0);
        assert(strcmp(Morse_Encode((char)('a'+i)), expected[i]) == 0);
        assert(Morse_Decode(expected[i]) == 'A'+(int)i);
    }
    assert(!Morse_Encode('+') && !Morse_Encode((char)255));
    assert(!Morse_Decode("") && !Morse_Decode("........") && !Morse_Decode("x"));
}
static void test_morse(void)
{
    MorseState s; uint8_t v;
    Morse_Init(&s, 0);
    assert(Morse_Enqueue(&s, 'A', 0));
    for (unsigned i=0;i<8;++i) assert(Morse_Enqueue(&s, 'E',0));
    assert(!Morse_Enqueue(&s,'T',0) && s.letters.count==8 && s.current=='A');
    Morse_Input(&s,'.',0);
    Morse_Update(&s,199,false); assert(s.green);
    Morse_Update(&s,200,false); assert(!s.green && !s.feedback);
    Morse_Update(&s,400,false); assert(s.green);
    Morse_Update(&s,999,false); assert(s.green);
    Morse_Update(&s,1000,false); assert(!s.green);
    Morse_Update(&s,1599,false); assert(!s.green);
    Morse_Update(&s,1600,false); assert(s.green && s.current=='E' && s.letters.count==7);
    Morse_Update(&s,1999,false); assert(!Ring_Peek(&s.results,&v));
    Morse_Update(&s,2000,false); assert(Ring_Read(&s.results,&v) && v=='E');

    Morse_Init(&s,0);
    Morse_Input(&s,'.',0);
    Morse_Update(&s,5000,true); assert(s.results.count==0);
    Morse_Input(&s,'-',5100);
    Morse_Update(&s,7099,false); assert(s.results.count==0);
    Morse_Update(&s,7100,false); assert(Ring_Read(&s.results,&v) && v=='A');
    for(unsigned i=0;i<9;++i) Morse_Input(&s,'.',7200+i*100);
    Morse_Update(&s,10000,false);
    assert(Ring_Read(&s.results,&v) && v==MORSE_INPUT_OVERFLOW);
    for(unsigned i=0;i<5;++i) Morse_Input(&s,'.',10100+i*100);
    Morse_Update(&s,12500,false); assert(Ring_Read(&s.results,&v) && v==MORSE_INVALID);
    Morse_Init(&s,UINT32_MAX-100);
    assert(Morse_Enqueue(&s,'E',UINT32_MAX-100));
    Morse_Input(&s,'.',UINT32_MAX-100);
    Morse_Update(&s,98,false); assert(s.green);
    Morse_Update(&s,99,false); assert(!s.green);
    Morse_Update(&s,1899,false); assert(Ring_Read(&s.results,&v) && v=='E');
}
static ButtonEvent button_at(uint32_t t,uint8_t level)
{ fake_tick=t;fake_button_level=level;return Button_Update(); }
static void test_button(void)
{
    Fake_Reset(); Button_Init();
    assert(button_at(0,0)==BUTTON_EVENT_NONE);
    assert(button_at(10,1)==BUTTON_EVENT_NONE);
    assert(button_at(20,0)==BUTTON_EVENT_NONE);
    assert(button_at(49,0)==BUTTON_EVENT_NONE && !Button_IsPressed());
    assert(button_at(50,0)==BUTTON_EVENT_NONE && Button_IsPressed());
    assert(button_at(100,1)==BUTTON_EVENT_NONE);
    assert(button_at(129,1)==BUTTON_EVENT_NONE);
    assert(button_at(130,1)==BUTTON_EVENT_SHORT);
    button_at(200,0); button_at(230,0);
    assert(button_at(800,1)==BUTTON_EVENT_NONE);
    assert(button_at(830,1)==BUTTON_EVENT_LONG);
    button_at(900,0); button_at(930,0);
    button_at(1499,1); assert(button_at(1529,1)==BUTTON_EVENT_SHORT);
    Fake_Reset(); fake_tick=UINT32_MAX-50;Button_Init();
    button_at(UINT32_MAX-50,0);button_at(UINT32_MAX-20,0);
    button_at(30,1);assert(button_at(60,1)==BUTTON_EVENT_SHORT);
}
static void service_tx(void)
{ Fake_CompleteTransmit();UART_DriverService(); }
static void test_uart(void)
{
    uint8_t b;
    Fake_Reset();UART_BoardStart();
    assert(UART_DriverPeek(&b)==UART_NO_DATA);
    assert(UART_DriverWrite((const uint8_t*)"ABC",3));
    UART_DriverService();
    assert(UART_DriverRequestMode(false));
    Fake_Receive('x');
    UART_DriverService();assert(UART_DriverSwitching());
    service_tx(); /* A completes; switch; B written, HAL_TIMEOUT after acceptance */
    assert(!UART_DriverInterrupts() && !fake_irq_enabled);
    assert(UART_DriverRead(&b)==UART_DATA_READY && b=='x');
    service_tx();service_tx();
    assert(fake_output_length==3 && memcmp(fake_output,"ABC",3)==0);
    assert(fake_uart.Instance->CR1==0 && fake_uart.Instance->CR3==0);
    assert(UART_DriverWrite((const uint8_t*)"D",1));UART_DriverService();
    assert(UART_DriverRequestMode(true));
    Fake_Receive('y');UART_DriverService();
    assert(UART_DriverSwitching()); /* TC still low */
    service_tx();
    assert(UART_DriverInterrupts() && fake_irq_enabled);
    assert(UART_DriverRead(&b)==UART_DATA_READY && b=='y');
    assert(fake_output_length==4 && memcmp(fake_output,"ABCD",4)==0);
    for(unsigned i=0;i<UART_RX_CAPACITY+1;++i) Fake_Receive((uint8_t)i);
    assert(UART_DriverDiagnostics().rx_overflow==1);
    for(unsigned i=0;i<UART_RX_CAPACITY;++i)
        assert(UART_DriverRead(&b)==UART_DATA_READY && b==(uint8_t)i);
    fake_uart.ErrorCode=1;
    Fake_Receive('!');HAL_UART_ErrorCallback(&fake_uart);UART_DriverService();
    assert(UART_DriverPeek(&b)==UART_NO_DATA);
    assert(UART_DriverDiagnostics().line_errors==1);
    Fake_Receive('z');assert(UART_DriverRead(&b)==UART_DATA_READY && b=='z');
    assert(UART_DriverRequestMode(false));service_tx();
    fake_tx_status=HAL_ERROR;assert(UART_DriverWrite((const uint8_t*)"E",1));
    UART_DriverService();assert(UART_DriverDiagnostics().tx_errors>0);
    fake_tx_status=HAL_TIMEOUT;UART_DriverService();service_tx();
    assert(fake_output_length==5 && fake_output[4]=='E');
}
static void pump_app(void)
{
    for(unsigned i=0;i<300;++i){App_Update();Fake_CompleteTransmit();}
}
static void protocol(void)
{
    Fake_Reset();App_Init();pump_app();
    const uint8_t input[]={'a','\r','\n','?','+','b','+'};
    for(unsigned i=0;i<sizeof(input);++i){Fake_Receive(input[i]);pump_app();}
}
static void test_app(void)
{
    protocol();
    const char expected[]="\r\ninterrupts on\r\na\r\ntransmitting symbol: A\r\n\r\n?\r\nerror: unsupported input\r\n+\r\ninterrupts off\r\nb\r\ntransmitting symbol: B\r\n+\r\ninterrupts on\r\n";
    assert(fake_output_length==strlen(expected) && memcmp(fake_output,expected,strlen(expected))==0);
    /* Real button driver and application, while A is still playing. */
    fake_button_level=0;fake_tick=0;App_Update();
    fake_tick=30;App_Update();fake_tick=100;fake_button_level=1;App_Update();
    fake_tick=130;App_Update();assert(fake_feedback=='.');
    fake_tick=2130;App_Update();pump_app();
    fake_output[fake_output_length]=0;
    assert(strstr((const char*)fake_output,"received symbol: E")!=NULL);

    /* A full TX queue must defer the command, not accept it without its ACK. */
    Fake_Reset();App_Init();pump_app();
    uint8_t filling[UART_TX_CAPACITY];memset(filling,'x',sizeof(filling));
    assert(UART_DriverWrite(filling,sizeof(filling)));
    Fake_Receive('A');App_Update();
    uint8_t pending;
    assert(UART_DriverPeek(&pending)==UART_DATA_READY && pending=='A');
    assert(!fake_green);
    for(unsigned i=0;i<10;++i)pump_app();
    assert(UART_DriverPeek(&pending)==UART_NO_DATA && fake_green);
    fake_output[fake_output_length]=0;
    const char *reply=strstr((const char*)fake_output,"A\r\ntransmitting symbol: A\r\n");
    assert(reply && !strstr(reply+1,"transmitting symbol: A\r\nA"));
}
int main(int argc,char **argv)
{
    if(argc>1 && strcmp(argv[1],"protocol")==0){
        protocol();for(size_t i=0;i<fake_output_length;++i)printf("%02x",fake_output[i]);puts("");return 0;
    }
    test_ring(); puts("PASS ring: full capacity, atomic write, wraparound, FIFO");
    test_codec(); puts("PASS codec: A-Z reference table and invalid input");
    test_morse(); puts("PASS Morse: queue, independent input, timing, overflow, tick wrap");
    test_button(); puts("PASS button: debounce, short/long boundary, tick wrap");
    test_uart(); puts("PASS UART: timeout acceptance, modes, pending TX/RX, errors");
    test_app(); puts("PASS application: exact protocol and simultaneous button input");
    return 0;
}
