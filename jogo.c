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
void check_tokens();
void remove_token(int position_to_remove);

char board[LINES][COLS];

int current_tokens = TOKENS;

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
int sleep_timing;

// -- MAIN --------------------------------------------------------------------
int main(void) {

  int cursor_input;

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
  
  /* inicializa tabuleiro */
  draw_board();
 
  /* move os tokens aleatoriamente */
  move_tokens();
 
  do { 
    board_refresh();
    
    cursor_input = getch();
    apply_player_cursor_change(cursor_input);
    /* TODO:
      Adicionar função para verificar se algum token deve ser removido
    */
    check_tokens();
  } while ((cursor_input != 'q') && (cursor_input != 'Q'));
  endwin();
  exit(0);
}

// -- END MAIN ----------------------------------------------------------------

void move_tokens() {
  int i;
  int *ids[current_tokens];
  
  /* Cria as threads e passa função para cada thread movimentar seu token no tabuleiro*/
  for (i = 0; i < current_tokens; i++) {
    ids[i] = malloc(sizeof(int));
    *ids[i] = i;

    pthread_create(&tokens[i], NULL, move_token, (void *)ids[i]);
  }
}

void board_refresh(void) {
  int x, y, i;

  pthread_mutex_lock(&board_mutex); // ----------------------------------------
  /* redesenha tabuleiro "limpo" */                                        // -
  for (x = 0; x < COLS; x++)                                               // -
    for (y = 0; y < LINES; y++){                                           // -
      attron(COLOR_PAIR(EMPTY_PAIR));                                      // -
      mvaddch(y, x, EMPTY);                                                // -
      attroff(COLOR_PAIR(EMPTY_PAIR));                                     // -
  }                                                                        // -
  pthread_mutex_unlock(&board_mutex); // --------------------------------------

  /* poe os tokens no tabuleiro */
  for (i = 0; i < current_tokens; i++) {
    attron(COLOR_PAIR(TOKEN_PAIR));
    mvaddch(coord_tokens[i].y, coord_tokens[i].x, EMPTY);
    attroff(COLOR_PAIR(TOKEN_PAIR));
  }
  
  pthread_mutex_lock(&cursor_mutex); // ---------------------------------------
  //poe o cursor no tabuleiro                                              // -
  move(y, x);                                                              // -
  refresh();                                                               // -
  attron(COLOR_PAIR(CURSOR_PAIR));                                         // -
  mvaddch(cursor.y, cursor.x, EMPTY);                                      // -
  attroff(COLOR_PAIR(CURSOR_PAIR));                                        // -
  pthread_mutex_unlock(&cursor_mutex); // -------------------------------------
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
  do {
    current_time = time(NULL);
    if (current_time - start_time >= 1){
      start_time = current_time;
   
      pthread_mutex_lock(&board_mutex); // ------------------------------------
                                                                           // -
      pthread_mutex_lock(&cursor_mutex); // ----------------------------   // -
      do {                                                          // -   // -
        new_x = rand()%(COLS);                                      // -   // -
        new_y = rand()%(LINES);                                     // -   // -
      } while (                                                     // -   // -
        (board[new_x][new_y] != 0)                                  // -   // -
        || ((new_x == cursor.x) && (new_y == cursor.y))             // -   // -
      );                                                            // -   // -
      pthread_mutex_unlock(&cursor_mutex); // --------------------------   // -
                                                                           // -
      /* retira token da posicao antiga  */                                // -
      board[coord_tokens[i].x][coord_tokens[i].y] = 0;                     // -
      board[new_x][new_y] = i;                                             // -
      pthread_mutex_unlock(&board_mutex); // ----------------------------------
   
      /* coloca token na nova posicao */ 
      coord_tokens[i].x = new_x;
      coord_tokens[i].y = new_y;
  
      /* redesenha tabuleiro */
      board_refresh(); 
   
      /* TODO:
        Ajustar velocidade de movimentação dos tokens de acordo com dificuldade
        mudando o sleep_timing

        comando sleep para a thread
      */
      sleep(sleep_timing);
    }
  } while (TRUE);
}

void draw_board(void) {
  int x, y;

  /* limpa matriz que representa o tabuleiro */
  for (x = 0; x < COLS; x++) {
    for (y = 0; y < LINES; y++) {
      pthread_mutex_lock(&board_mutex); // ------------------------------------
      board[x][y] = 0;                                                     // -
      pthread_mutex_unlock(&board_mutex); // ----------------------------------
    }
  }
}

void apply_player_cursor_change(int cursor_input) {
  pthread_mutex_lock(&cursor_mutex);  // --------------------------------------
  switch (cursor_input) {                                                  // -
    case KEY_UP:                                                           // -
    case 'w':                                                              // -
    case 'W':                                                              // -
      if ((cursor.y > 0)) {                                                // -
        cursor.y = cursor.y - 1;                                           // -
      }                                                                    // -
      break;                                                               // -
    case KEY_DOWN:                                                         // -
    case 's':                                                              // -
    case 'S':                                                              // -
      if ((cursor.y < LINES - 1)) {                                        // -
        cursor.y = cursor.y + 1;                                           // -
      }                                                                    // -
      break;                                                               // -
    case KEY_LEFT:                                                         // -
    case 'a':                                                              // -
    case 'A':                                                              // -
      if ((cursor.x > 0)) {                                                // -
        cursor.x = cursor.x - 1;                                           // -
      }                                                                    // -
      break;                                                               // -
    case KEY_RIGHT:                                                        // -
    case 'd':                                                              // -
    case 'D':                                                              // -
      if ((cursor.x < COLS - 1)) {                                         // -
        cursor.x = cursor.x + 1;                                           // -
      }                                                                    // -
      break;                                                               // -
  }                                                                        // -
  pthread_mutex_unlock(&cursor_mutex); // -------------------------------------
}

void check_tokens() {
  int token_position;
  pthread_mutex_lock(&cursor_mutex); // ---------------------------------------
  for (token_position = 0; token_position < current_tokens; token_position++) {
    if (                                                                   // -
      coord_tokens[token_position].y == cursor.y                           // -
      && coord_tokens[token_position].x == cursor.x                        // -
    ) {                                                                    // -
      remove_token(token_position);                                        // -
    }                                                                      // -
  }                                                                        // -
  pthread_mutex_unlock(&cursor_mutex); // -------------------------------------
}

void remove_token(int position_to_remove){
  int position;
  for (position = position_to_remove - 1; position < current_tokens - 1; position++){
    coord_tokens[position_to_remove] = coord_tokens[position + 1];
  }
  current_tokens = current_tokens - 1;
}

void input_difficulty() {
  int choice;

  printf("Escolha uma dificuldade\n\n1 - Fácil\n2 - Médio\n3 - Difícil\n");
  scanf("%d", &choice);

  switch(choice) {
    case 1:
      sleep_timing = 1;
    break;
    case 2:
      sleep_timing = 0.5;
    break;
    case 3:
      sleep_timing = 0.2;
    break;
    default:
      printf("Opção inválida. Escolha novamente!\n\n");
      input_difficulty();
    break;
  }
}
