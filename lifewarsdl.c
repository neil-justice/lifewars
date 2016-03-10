/* LIFE WARS - a 2-player game adapted from Conway's Game of Life.
 * The game board is an array of structs */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "neillsdl2.h"

#define HEIGHT WHEIGHT / RECTSIZE
#define WIDTH WWIDTH / RECTSIZE

#define FHEADER "#life 1.06"
#define FHEADERLEN 20 /* length of str to store header from .lif file */
#define MAXLEN 2300 /* no. of lines allowed in .lif file */
#define GENNUM 5000 /* no. of generations per game */
#define GAMENUM 50 /* no. of games */
#define NCELLS 8 /* no. of neighbouring cells each cell has */
#define UP    1 /* directions for the neighbour-finding function */
#define DOWN -1
#define LEFT -1
#define RIGHT 1

#define RECTSIZE 3
#define MILLISECONDDELAY 50

typedef enum cellstatus {dead, alive} cellstatus;
typedef enum cellteam {noteam, player1, player2} cellteam;
typedef struct cell {
  cellstatus  status;
  cellstatus  nextstatus;
  short       neighbours; /* no. of neighbours */
  struct cell *n[NCELLS]; /* array of pointers to neighbours */
  cellteam    team;
} cell;

void SDLprintArray(cell a[HEIGHT][WIDTH], struct SDL_Simplewin *sw);
void displayGame(cell a[HEIGHT][WIDTH],SDL_Simplewin *sw);

void clearArray(cell a[HEIGHT][WIDTH]);
void populateNeighbourArray(cell a[HEIGHT][WIDTH]);
void countLiveNeighbours(cell a[HEIGHT][WIDTH]);
void calcRules(cell a[HEIGHT][WIDTH]);
void updateCells(cell a[HEIGHT][WIDTH]);
int  locateNeighbour(int coord, int min, int max, int direction);
int  countLiveCells(cell a[HEIGHT][WIDTH], cellteam team);
void updateCellTeams(cell a[HEIGHT][WIDTH]);
cellteam findBirthTeam(cell a[HEIGHT][WIDTH], int y, int x);

void handleInput(int argc, char **argv, cellteam team, cell a[HEIGHT][WIDTH]);
void clearTeamCells(cell a[HEIGHT][WIDTH], cellteam team);
int  createPattern(cell a[HEIGHT][WIDTH], cellteam team, FILE *file);
int  checkStartCoords(int coord, int min, int max);
void checkVersion(FILE *file);

void printResults(cell a[HEIGHT][WIDTH]);
void printFinalResults(int p1cnt, int p2cnt);
void printDebugInfo(cell a[HEIGHT][WIDTH], int y, int x);
int  randGen(int mod);
void lowerCase( char *s);

int main(int argc, char **argv)
{
  cell a[HEIGHT][WIDTH];
  SDL_Simplewin sw; /*see Neill's .h file for struct definition */

  /* initialise and read data*/
  Neill_SDL_Init(&sw);
  srand(time(NULL));
  populateNeighbourArray(a);

  fprintf(stdout,"\nLIFE WARS\n\n");
  fprintf(stdout,
  "(Any lines after line %d in a .lif file will be discarded)\n", MAXLEN);
  fprintf(stdout,"\n%4s%4s %5s  %s\n", "game","P1","P2","Winner");

  clearArray(a);
  handleInput(argc, argv, player1, a);
  handleInput(argc, argv, player2, a);
  do {
    countLiveNeighbours(a);
    calcRules(a);
    displayGame(a,&sw);
    updateCellTeams(a);
    updateCells(a);
  }
  while(!sw.finished);
  printResults(a);

  atexit(SDL_Quit);
  return 0;
}

void displayGame(cell a[HEIGHT][WIDTH],SDL_Simplewin *sw)
{
  SDL_Delay(MILLISECONDDELAY);
  SDLprintArray(a,sw);
  /* Update window - no graphics appear on some devices until this is finished */
  SDL_RenderPresent(sw->renderer);
  SDL_UpdateWindowSurface(sw->win);
  Neill_SDL_Events(sw); /*checks for keypress*/
  Neill_SDL_SetDrawColour(sw, 0, 0, 0);
  SDL_RenderClear(sw->renderer); /* clears screen */
}

void SDLprintArray(cell a[HEIGHT][WIDTH], SDL_Simplewin *sw)
{
  int x,y;
  SDL_Rect rectangle;
  rectangle.w = RECTSIZE;
  rectangle.h = RECTSIZE;

  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      if (a[y][x].status == 1 && a[y][x].team == player1) {
        Neill_SDL_SetDrawColour(sw,0,150,100);
        rectangle.x = x * RECTSIZE;
        rectangle.y = y * RECTSIZE;
        SDL_RenderFillRect(sw->renderer, &rectangle);
  	  }
      if (a[y][x].status == 1 && a[y][x].team == player2) {
        Neill_SDL_SetDrawColour(sw,150,0,0);
        rectangle.x = x * RECTSIZE;
        rectangle.y = y * RECTSIZE;
        SDL_RenderFillRect(sw->renderer, &rectangle);
  	  }
    }
  }
}

void clearArray(cell a[HEIGHT][WIDTH])
{
  int x,y;

  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      a[y][x].status = dead;
      a[y][x].nextstatus = dead;
      a[y][x].team = noteam;
      a[y][x].neighbours = 0;
    }
  }
}

void populateNeighbourArray(cell a[HEIGHT][WIDTH])
{
  int x, y;
  int lx, rx, ay, by;
  /* lx: left of x; rx: right of x; ay: above y; by: below y */

  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      by = locateNeighbour(y, 0, HEIGHT - 1, DOWN);
      ay = locateNeighbour(y, 0, HEIGHT - 1, UP);
      lx = locateNeighbour(x, 0, WIDTH - 1, LEFT);
      rx = locateNeighbour(x, 0, WIDTH - 1, RIGHT);
      a[y][x].n[0] = &a[by][lx]; /* array of pointers to neighbours */
      a[y][x].n[1] = &a[by][x];
      a[y][x].n[2] = &a[by][rx];
      a[y][x].n[3] = &a[y][lx];
      a[y][x].n[4] = &a[y][rx];
      a[y][x].n[5] = &a[ay][lx];
      a[y][x].n[6] = &a[ay][x];
      a[y][x].n[7] = &a[ay][rx];
    }
  }
}

int locateNeighbour(int coord, int min, int max, int direction)
{
  /* returns toroidal neighbour */
  int neighbour = coord + direction;

  if (coord == min && direction < 0) {
    neighbour = max;
  }
  if (coord == max && direction > 0) {
    neighbour = min;
  }
  return neighbour;
}

void countLiveNeighbours(cell a[HEIGHT][WIDTH])
{
  int x, y, i;

  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      a[y][x].neighbours = 0;
      for (i = 0; i < NCELLS; i++) {
        a[y][x].neighbours += a[y][x].n[i]->status;
      }
    }
  }
}

void calcRules(cell a[HEIGHT][WIDTH]){
  int x,y;
  /* survive: 2,3   neighbours, self = 1
   * die:     1,4-8 neighbours, self = 1
   * born:    3     neighbours, self = 0 */
  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      switch (a[y][x].neighbours) {
        case 0:
          a[y][x].nextstatus = dead;
          break;
        case 1:
          a[y][x].nextstatus = dead;
          break;
        case 2:
          a[y][x].nextstatus = a[y][x].status;
          break;
        case 3:
          a[y][x].nextstatus = alive;
          break;
        case 4:
          a[y][x].nextstatus = dead;
          break;
        case 5:
          a[y][x].nextstatus = dead;
          break;
        case 6:
          a[y][x].nextstatus = dead;
          break;
        case 7:
          a[y][x].nextstatus = dead;
          break;
        case 8:
          a[y][x].nextstatus = dead;
          break;
        default:
          fprintf(stderr,"neighbour count wrong - probably toroid ");
          fprintf(stderr,"wrapping failure or array out of bounds\n");
          printDebugInfo(a,y,x);
      }
    }
  }
}

cellteam findBirthTeam(cell a[HEIGHT][WIDTH], int y, int x)
{
  int i, t1cnt = 0, t2cnt = 0;

  for(i = 0; i < NCELLS; i++) {
    if(a[y][x].n[i]->team == player1
    && a[y][x].n[i]->status == alive) {
      t1cnt++;
    }
    if(a[y][x].n[i]->team == player2
    && a[y][x].n[i]->status == alive) {
      t2cnt++;
    }
  }
  if (t1cnt > t2cnt) {
    return player1;
  }
  else if (t2cnt > t1cnt) {
    return player2;
  }
  else {
    fprintf(stderr,"no birth team found for cell at (%d,%d)\n", x,y);
    fprintf(stderr,"t1 count: %d\nt2 count: %d\n", t1cnt, t2cnt);
    printDebugInfo(a,y,x);
    return noteam;
  }
}

void updateCellTeams(cell a[HEIGHT][WIDTH])
{
  int x,y;

  /* sets team for newborn cells */
  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      if (a[y][x].nextstatus == alive
      && a[y][x].status == dead) {
        a[y][x].team = findBirthTeam(a,y,x);
      }
    }
  }
  /* sets team to zero on dying cells */
  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      if (a[y][x].nextstatus == dead) {
        a[y][x].team = noteam;
     }
    }
  }
}

void updateCells(cell a[HEIGHT][WIDTH])
{
  int x,y;

  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      a[y][x].status = a[y][x].nextstatus;
    }
  }
}

int countLiveCells(cell a[HEIGHT][WIDTH], cellteam team)
{
  int x, y, sum = 0;

  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      if (a[y][x].team == team) {
        sum += a[y][x].status;
      }
    }
  }
  return sum;
}

void handleInput(int argc, char **argv, cellteam team, cell a[HEIGHT][WIDTH])
{
  FILE *file;
  int collision;

  do {
    file = fopen(argv[team], "r");

    if (file == NULL) {
      fprintf(stderr, "Error opening file - check name and directory.\n");
      exit(1);
    }
    if (argc != 3) {
      fprintf(stderr, "Invalid number of arguments supplied.\n");
      fclose(file);
      exit(1);
    }
    checkVersion(file);

    collision = createPattern(a, team, file);
    if (collision != 0) {
      clearTeamCells(a,team);
    }
    fclose(file);
  }
  while (collision != 0);
}

void clearTeamCells(cell a[HEIGHT][WIDTH], cellteam team)
{
  int x, y;

  for (y = 0; y < HEIGHT; y++) {
    for (x = 0; x < WIDTH; x++) {
      if (a[y][x].team == team) {
        a[y][x].status = dead;
        a[y][x].team = noteam;
      }
    }
  }
}

int createPattern(cell a[HEIGHT][WIDTH], cellteam team, FILE *file)
{
  int x, y, i, xrandoffset, yrandoffset, error = 0;

  xrandoffset = randGen(WIDTH);
  yrandoffset = randGen(HEIGHT);

  for (i = 0; i < MAXLEN; i++) {
    if (fscanf(file,"%d %d", &x, &y) == 2) {
      x = checkStartCoords(x + xrandoffset, 0, WIDTH);
      y = checkStartCoords(y + yrandoffset, 0, HEIGHT);
      if (a[y][x].status == dead) {
        a[y][x].status = alive;
        a[y][x].team = team;
      }
      else {
        error += 1; /* collision detection */
      }
    }
  }
  return error;
}

int randGen(int mod)
{
  return (rand() % mod );
}

int checkStartCoords(int coord, int min, int max)
{
/* Checks for coordinates outside the board (e.g. -1)
 * in .lif file and attempts to convert them.
 * Will probably fail if coord > 2* max etc */
  while (coord < min) {
    coord += max;
  }
  while (coord > max - 1) {
    coord -= max;
  }
  return coord;
}

void checkVersion(FILE *file)
{
  char fileheader[FHEADERLEN];

  if (fgets(fileheader, FHEADERLEN, file) == NULL) {
    fprintf(stderr, "Failure to read header from .lif file.\n");
    fclose(file);
    exit(1);
  }
  lowerCase(fileheader);
  fileheader[strlen(FHEADER)] = '\0';
  if (strcmp(fileheader, FHEADER) != 0) {
    fprintf(stderr, "%s %s\n",
    "File header indicates this is not a life 1.06 file.  it reads:",
    fileheader);
    fprintf(stderr, "Strcmp output: %d\n", strcmp(fileheader, FHEADER));
    fclose(file);
    exit(1);
  }
}

void lowerCase( char *s)
{
  int i;

  for (i = 0; s[i]; i++)  {
    s[i] = tolower(s[i]);
  }
}

void printResults(cell a[HEIGHT][WIDTH])
{
  static int p1cnt = 0, p2cnt = 0, cnt = 0;
  cellteam winner = noteam;

  cnt++;
  p1cnt += countLiveCells(a, player1);
  p2cnt += countLiveCells(a, player2);

  fprintf(stdout,"%2d %5d %5d ",cnt, p1cnt, p2cnt);
  if (p1cnt > p2cnt) {
    winner = player1;
    fprintf(stdout,"Player %d\n", winner);
  }
  else if (p2cnt > p1cnt) {
    winner = player2;
    fprintf(stdout,"Player %d\n", winner);
  }
  else {
    fprintf(stdout,"Draw\n");
  }

  if (cnt == GAMENUM) {
    printFinalResults(p1cnt, p2cnt);
  }
}

void printFinalResults(int p1cnt, int p2cnt)
{
  cellteam winner = noteam;

  fprintf(stdout,"\n");
	if (p1cnt > p2cnt) {
	  winner = player1;
	  fprintf(stdout,"Player %d wins by %d cells to %d cells",
	  winner, p1cnt, p2cnt);
	}
	else if (p2cnt > p1cnt) {
	  winner = player2;
	  fprintf(stdout,"Player %d wins by %d cells to %d cells",
	  winner, p2cnt, p1cnt);
	}
	else {
	  fprintf(stdout,"draw, %d cells to %d cells",
	  p2cnt, p1cnt);
	}
  fprintf(stdout,"\n\n");
}

void printDebugInfo(cell a[HEIGHT][WIDTH], int y, int x)
{
  int i;

  fprintf(stderr,"\n*** debug info for cell %3d,%3d ***\n",
  x,y);
  fprintf(stderr,"* status: %d\n", a[y][x].status);
  fprintf(stderr,"* next: %d\n", a[y][x].nextstatus);
  fprintf(stderr,"* neighbours: %d\n", a[y][x].neighbours);
  fprintf(stderr,"* team: %d\n", a[y][x].team);
  fprintf(stderr,"*\n");
  fprintf(stderr,"* living neighbour info:\n");
  for (i = 0; i < NCELLS; i++) {
    if (a[y][x].n[i]->status == alive) {
      fprintf(stderr,"* n[%d] status: %d\n",
      i,a[y][x].n[i]->status);
      fprintf(stderr,"* n[%d] next: %d\n",
      i,a[y][x].n[i]->nextstatus);
      fprintf(stderr,"* n[%d] neighbours: %d\n",
      i,a[y][x].n[i]->neighbours);
      fprintf(stderr,"* n[%d] team: %d\n",
      i,a[y][x].n[i]->team);
      fprintf(stderr,"*\n");
    }
  }
  fprintf(stderr,"***********************************\n\n");
}
