#include "drpc_server.h"
#include "drpc_client.h"
#include "drpc_queue.h"
#include "drpc_struct.h"
#include "drpc_types.h"

#include <assert.h>
#include <stdio.h>
#include <termios.h>



int main(void){
    struct drpc_client* client = drpc_client_connect("127.0.0.1",2077,"game_client","");
    assert(client != NULL);


    int c;
    static struct termios oldt, newt;

    /*tcgetattr gets the parameters of the current terminal
     S TDIN_FILENO will tell tcgetattr that it *should write the settings
     of stdin to oldt*/
    tcgetattr( STDIN_FILENO, &oldt);
    /*now the settings will be copied*/
    newt = oldt;

    /*ICANON normally takes care that one line at a time will be processed
     that means it will return if it sees a "\*n" or an EOF or an EOL*/
    newt.c_lflag &= ~(ICANON);

    /*Those new settings will be set to STDIN
     TCSANOW tells tcsetattr to change attribu*tes immediately. */
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);

    /*This is your part:
     I choose 'e' to end input. Notice that EO*F is also turned off
     in the non-canonical mode*/
    while((c=getchar())!= ']'){
        struct d_struct* massage = new_d_struct();
        d_struct_set(massage,"input",&c,d_char);

        assert(drpc_client_send_delayed(client,"input_receiver",massage) == 0);
        d_struct_free(massage);
    }

    /*restore the old settings*/
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
}
