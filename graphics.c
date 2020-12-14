#include "defs.h"

void initNcurses()
{
  initscr();
  curs_set(0);
  clear();
}

void cleanupNcurses(int row)
{
  move(row, 0);
  addstr("Press any key to continue...");
  getch();
  endwin();
}

void scrPrt(char *str, int row, int col)
{
  move(row, col);
  addstr(str);
  refresh();
}

int randm(int max)
{
  double r = ( (double)rand() / ((double)(RAND_MAX)+(double)(1)) ); 
  return (int)(r * max);
}