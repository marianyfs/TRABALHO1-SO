#include <curses.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>


#define EMPTY     ' '
#define CURSOR_PAIR   1
#define TOKEN_PAIR    2
#define EMPTY_PAIR    3
#define LINES 11
#define COLS 11
#define TOKENS 5

int is_move_okay(int y, int x);
void draw_board(void);
void *move_token(void *arg);
void move_tokens();
void board_refresh(void);

char board[LINES][COLS];

pthread_t tokens[TOKENS]; /* declaração do vetor de threads (tokens) */
pthread_mutex_t mutex1; /* declaração do mutex para proteger região crítica */

typedef struct CoordStruct {
  int x;
  int y;
} coord_type;
 
coord_type cursor, coord_tokens[TOKENS];
int tempoSleep;
int escolha;
int tempo;
int continua = 1;
time_t start, current;

  
int main(void) {
  int ch;
  srand(time(NULL));  /* inicializa gerador de numeros aleatorios */ 
  

  while(continua == 1) {
  
  printf("Escolha uma dificuldade\n\n");
  printf("1 - Fácil\n");
  printf("2 - Médio\n");
  printf("3 - Difícil\n");
  scanf("%d", &escolha);
  
  switch(escolha) {
   
    case 1:
    tempo = 60;
    tempoSleep = 1;
    continua = 2;
    break;
    case 2:
    tempo = 45;
    tempoSleep = 0.5;
    continua = 2;
    break;
    case 3:
    tempo = 30;
    tempoSleep = 0.2;
    continua = 2;
    break;
    default:
    printf("Opção inválida. Escolha novamente!\n\n");
    break;
   }	
  }
  
  /* inicializa curses */
    
  initscr();
  keypad(stdscr, TRUE);
  cbreak();
  noecho();
  
  /* inicializa colors */

  if (has_colors() == FALSE) {
    endwin();
    printf("Seu terminal nao suporta cor\n");
    exit(1);
  }

    start_color();
    
  /* inicializa pares caracter-fundo do caracter */

  init_pair(CURSOR_PAIR, COLOR_YELLOW, COLOR_YELLOW);
  init_pair(TOKEN_PAIR, COLOR_RED, COLOR_RED);
  init_pair(EMPTY_PAIR, COLOR_BLUE, COLOR_BLUE);
  clear();
  
  
  pthread_mutex_init(&mutex1,NULL); /* inicializa mutex*/
  
  draw_board(); /* inicializa tabuleiro */
  
  /* comando de repetição para ajustar velocidade de movimentação dos tokens de acordo com dificuldade*/
 
    move_tokens();/* move os tokens aleatoriamente */
 
   do {
    pthread_mutex_lock(&mutex1);
    board_refresh();
    pthread_mutex_unlock(&mutex1);
    //marcar o tempo
    
    ch = getch();
    pthread_mutex_lock(&mutex1);
    switch (ch) {
      case KEY_UP:
      case 'w':
      case 'W':
        if ((cursor.y > 0)) {
          cursor.y = cursor.y - 1;
        }
        break;
      case KEY_DOWN:
      case 's':
      case 'S':
        if ((cursor.y < LINES - 1)) {
          cursor.y = cursor.y + 1;
        }
        break;
      case KEY_LEFT:
      case 'a':
      case 'A':
        if ((cursor.x > 0)) {
          cursor.x = cursor.x - 1;
        }
        break;
      case KEY_RIGHT:
      case 'd':
      case 'D':
        if ((cursor.x < COLS - 1)) {
          cursor.x = cursor.x + 1;
        }
        break;
    }
   pthread_mutex_unlock(&mutex1);
   // criar funcao para verificar posicao do cursor e matar a thread
    
  }while ((ch != 'q') && (ch != 'Q'));
  endwin();
  exit(0);
 
  
}

void move_tokens() {
  int i;
  int *ids[TOKENS];
  
  /* Cria as threads e passa função para cada thread movimentar seu token no tabuleiro*/
  for (i = 0; i < TOKENS; i++) {
      ids[i] = malloc(sizeof(int));
      *ids[i] = i;
      pthread_create(&tokens[i],NULL,move_token,(void *)ids[i]);
}

  /* Aguarda término das threads*/
  //for (i = 0; i < TOKENS; i++)
     // pthread_join(tokens[i],NULL);
  
}

void board_refresh(void) {
  int x, y, i;

  /* redesenha tabuleiro "limpo" */

  for (x = 0; x < COLS; x++) 
    for (y = 0; y < LINES; y++){
      attron(COLOR_PAIR(EMPTY_PAIR));
      mvaddch(y, x, EMPTY);
      attroff(COLOR_PAIR(EMPTY_PAIR));
  }

  /* poe os tokens no tabuleiro */

  for (i = 0; i < TOKENS; i++) {
    attron(COLOR_PAIR(TOKEN_PAIR));
    mvaddch(coord_tokens[i].y, coord_tokens[i].x, EMPTY);
    attroff(COLOR_PAIR(TOKEN_PAIR));
  }
  
  //poe o cursor no tabuleiro 

    move(y, x);
    refresh();
    attron(COLOR_PAIR(CURSOR_PAIR));
    mvaddch(cursor.y, cursor.x, EMPTY);
    attroff(COLOR_PAIR(CURSOR_PAIR)); 
    
     
}

void *move_token(void *arg) {
  int i;
  
  int *id;
  id = (int *)arg;
  i = *id;
  int new_x, new_y; 
   
    start = time(NULL);
  for(;;) {
  current = time(NULL);
  if (current - start >= 1){
   start = current;
   
   /* Mutex para proteger a região crítica, nesse caso cada thread precisa executar o código que altera o tabuleiro de forma exclsiva
   sem que outra thread interfira*/
   pthread_mutex_lock(&mutex1);
   
  /* determina novas posicoes (coordenadas) do token no tabuleiro (matriz) */
  do{
    new_x = rand()%(COLS);
    new_y = rand()%(LINES);
  } while((board[new_x][new_y] != 0) || ((new_x == cursor.x) && (new_y == cursor.y)));
  
  /* retira token da posicao antiga  */ 
  board[coord_tokens[i].x][coord_tokens[i].y] = 0; 
  board[new_x][new_y] = i; 
   
  /* coloca token na nova posicao */ 
  coord_tokens[i].x = new_x;
  coord_tokens[i].y = new_y;
  
  /* redesenha tabuleiro */
  board_refresh(); 
  
  /* Destravando o mutex para que a próxima thread consiga executar o código */
  pthread_mutex_unlock(&mutex1);
   
  /* comando sleep para a thread */
  sleep(tempoSleep);
  
   }
 } 
    
  /* Encerrando execução da thread*/
  //pthread_exit(NULL);
  
}

void draw_board(void) {
  int x, y;

  /* limpa matriz que representa o tabuleiro */
  for (x = 0; x < COLS; x++) 
    for (y = 0; y < LINES; y++) 
      board[x][y] = 0;
}
