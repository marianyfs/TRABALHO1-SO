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

void input_difficulty();
void apply_player_cursor_change(int cursor_input);

char board[LINES][COLS];

/* declaração do vetor de threads (tokens) */
pthread_t tokens[TOKENS];

/* declaração do mutex para proteger a região crítica do borad */
pthread_mutex_t board_mutex;
/* declaração do mutex para proteger a região crítica do cursor */
pthread_mutex_t cursor_mutex;

typedef struct CoordStruct {
  int x;
  int y;
} coord_type;
 
coord_type cursor, coord_tokens[TOKENS];

/* Variáveis para controlar a dificuldade do jogo */ 
int sleep_timming;
bool request_difficulty = TRUE;
  
int main(void) {

  int cursor_input;

  /* inicializa gerador de numeros aleatorios */ 
  srand(time(NULL));
  
  input_difficulty();
  
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
  
  /* inicializa mutex
    :board_mutex    Mutex para proteger a região crítica, nesse caso cada
                    thread precisa executar o código que altera o tabuleiro
                    de forma exclsiva sem que outra thread interfira
    :cursor_mutex   Mutex para proteger a região crítica, nesse caso cada
                    thread precisa executar o código que altera o cursor
                    de forma exclsiva sem que outra thread interfira
  */
  pthread_mutex_init(&board_mutex, NULL);
  pthread_mutex_init(&cursor_mutex, NULL);
  
  /* inicializa tabuleiro */
  draw_board();
  
  /* TODO:
    comando de repetição para ajustar velocidade de movimentação dos tokens de
    acordo com dificuldade
  */
 
  /* move os tokens aleatoriamente */
  move_tokens();
 
  do {
    pthread_mutex_lock(&board_mutex);
    board_refresh();
    pthread_mutex_unlock(&board_mutex);
    
    cursor_input = getch();
    apply_player_cursor_change(cursor_input);
    
  } while ((cursor_input != 'q') && (cursor_input != 'Q'));
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
  /* i e id são informações do token, new_x e new y são as novas coordenadas
    daquele token */
  int i, *id, new_x, new_y; 

  /* Timers usados para controlar tempo de execução das threads */
  time_t start_time, current_time;

  id = (int *)arg;
  i = *id;
  
  /* TODO:
    Mudar o start_time pra ser atribuido no início na main thread pra ir
    decrescendo e chegar no tempo limite, pro jogardor perder o jogo
  */
  start_time = time(NULL);

  // Infinit running
  for(;;) {
    current_time = time(NULL);
    if (current_time - start_time >= 1){
      start_time = current_time;
   
      pthread_mutex_lock(&board_mutex);
   
      /* determina novas posicoes (coordenadas) do token no tabuleiro (matriz) */
      do {
        new_x = rand()%(COLS);
        new_y = rand()%(LINES);
      } while ((board[new_x][new_y] != 0) || ((new_x == cursor.x) && (new_y == cursor.y)));
  
      /* retira token da posicao antiga  */ 
      board[coord_tokens[i].x][coord_tokens[i].y] = 0; 
      board[new_x][new_y] = i; 
   
      /* coloca token na nova posicao */ 
      coord_tokens[i].x = new_x;
      coord_tokens[i].y = new_y;
  
      /* redesenha tabuleiro */
      board_refresh(); 
  
      /* Destravando o mutex para que a próxima thread consiga executar o código */
      pthread_mutex_unlock(&board_mutex);
   
      /* comando sleep para a thread */
      sleep(sleep_timming);
    }
  } 
}

void draw_board(void) {
  int x, y;

  /* limpa matriz que representa o tabuleiro */
  for (x = 0; x < COLS; x++) 
    for (y = 0; y < LINES; y++) 
      board[x][y] = 0;
}

void apply_player_cursor_change(int cursor_input) {
  pthread_mutex_lock(&board_mutex);
  switch (cursor_input) {
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
  pthread_mutex_unlock(&board_mutex);
}

void input_difficulty() {
  int choice;

  printf("Escolha uma dificuldade\n\n1 - Fácil\n2 - Médio\n3 - Difícil\n");
  scanf("%d", &choice);

  switch(choice) {
    case 1:
      sleep_timming = 1;
      request_difficulty = FALSE;
    break;
    case 2:
      sleep_timming = 0.5;
      request_difficulty = FALSE;
    break;
    case 3:
      sleep_timming = 0.2;
      request_difficulty = FALSE;
    break;
    default:
      printf("Opção inválida. Escolha novamente!\n\n");
      input_difficulty();
    break;
  }
}
