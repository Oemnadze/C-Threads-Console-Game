#include "graphics.c"

//Runner thread
void* goRunner(void* elem){
	RunnerType* runner = (RunnerType*)elem;
	while(1) {
		if (runner->dead || runner->ent.currPos <= END_POS) {
			break;
		}
		int randNum = randm(10);
		int newPos = runner->ent.currPos;
		if (randNum < 7) { //70% probability
			int randStep = randm(4) + 1;
			newPos -= randStep;	
			runner->health -= randStep;
			if (runner->health < 0) {
				runner->health = 0;
			}
		}
		else {
			int randStep = randm(3) + 1;
			newPos += randStep;
		}
		if (newPos > START_POS)
			newPos = START_POS;
		sem_wait(&race->mutex);
		for (int i = 0; i < race->numDorcs; i++) {
			if (race->dorcs[i]->currPos == runner->ent.currPos && race->dorcs[i]->path == runner->ent.path) {
				if (runner->name[0] == 'T')
					scrPrt("STATUS:  collision between Timmy and dorc", race->statusRow++, STATUS_COL);
				else
					scrPrt("STATUS:  collision between Harold and dorc", race->statusRow++, STATUS_COL);
				runner->health -= 3;
				if (runner->health < 0) {
					runner->health = 0;
				}
				break;
			}
		}
		sem_post(&race->mutex);
		if (runner->health == 0) {
			sem_wait(&race->mutex);
			runner->ent.avatar[0] = '+';
			runner->dead = 1;
			if (runner->name[0] == 'T')
				scrPrt("STATUS:  Timmy is dead", race->statusRow++, STATUS_COL);
			else
				scrPrt("STATUS:  Harold is dead", race->statusRow++, STATUS_COL);
			sem_post(&race->mutex);
		}
		sem_wait(&race->mutex);
		scrPrt(" ", runner->ent.currPos, runner->ent.path);
		scrPrt(runner->ent.avatar, newPos, runner->ent.path);
		char numStr[3];
		if (runner->health > 9) { //2 digits
			numStr[2] = '\0';
			numStr[1] = (runner->health % 10) + '0';
			numStr[0] = (runner->health / 10) + '0';
			if (runner->name[0] == 'T')
				scrPrt(numStr, 2, 37);
			else
				scrPrt(numStr, 2, 40);
		}
		else {
			numStr[2] = '\0';
			numStr[1] = runner->health + '0';
			numStr[0] = ' ';
			if (runner->name[0] == 'T')
				scrPrt(numStr, 2, 37);
			else
				scrPrt(numStr, 2, 40);
		}
		sem_post(&race->mutex);
		runner->ent.currPos = newPos;
		usleep(250000);
	}
	if (!runner->dead && race->winner[0] == '\0') {
		race->winner[0] = runner->name[0];
	}
	if (runner->ent.avatar[0] != '+') {
		scrPrt(" ", runner->ent.currPos, runner->ent.path);
		scrPrt(runner->ent.avatar, END_POS, runner->ent.path);
	}
}

//Dorc thread
void* goDorc(void* elem) {
	EntityType* dorc = (EntityType*)elem;
	while (1) {
		if (dorc->currPos >= 35) {
			break;
		}
		int newRow = dorc->currPos + randm(5) + 1; //add random number in range 1..5
		int newCol = dorc->path + randm(5) - 2; //add random number in range -2..2
		if (newCol != PATH_1 && newCol != PATH_2 && newCol != (PATH_1 + PATH_2) / 2) {
			int dif1 = abs(newCol - PATH_1);
			int dif2 = abs(newCol - PATH_2);
			int dif3 = abs(newCol - (PATH_1 + PATH_2) / 2);
			if (dif1 >= dif2 && dif1 >= dif3) {
				newCol = PATH_1;
			} else if (dif2 >= dif1 && dif2 >= dif3) {
				newCol = PATH_2;
			} else if (dif3 >= dif1 && dif3 >= dif2) {
				newCol = (PATH_1 + PATH_2) / 2;
			}
		}
		sem_wait(&race->mutex);
		scrPrt(" ", dorc->currPos, dorc->path);
		scrPrt("d", newRow, newCol);
		sem_post(&race->mutex);
		dorc->currPos = newRow;
		dorc->path = newCol;
		usleep(700000);
	}
	scrPrt(" ", dorc->currPos, dorc->path);
}

void cleanUp() {
	for (int i = 0; i < race->numRunners; i++) {
		free(race->runners[i]);
	}
	for (int i = 0; i < race->numDorcs; i++) {
		free(race->dorcs[i]);
	}
	for (int i = 0; i < 36; i++) {
		cleanupNcurses(i);
	}
}

int main() {

	//Initialization
	race = malloc(sizeof(RaceInfoType));
	
	RunnerType* Timmy = malloc(sizeof(RunnerType));
	RunnerType* Harold = malloc(sizeof(RunnerType));
	Timmy->ent.avatar[0] = 'T';
	Timmy->ent.avatar[1] = '\0';
	Harold->ent.avatar[0] = 'H';	
	Harold->ent.avatar[1] = '\0';
	Timmy->ent.path = PATH_1;	
	Harold->ent.path = PATH_2;	
	Timmy->ent.currPos = START_POS;	
	Harold->ent.currPos = START_POS;
	Timmy->health = MAX_HEALTH; 
	Harold->health = MAX_HEALTH;
	Timmy->dead = 0; 
	Harold->dead = 0;
	Timmy->name[0]= 'T';
	Harold->name[0]= 'H';
	Timmy->name[1]= '\0';
	Harold->name[1]= '\0';
	race->runners[0] = Timmy;
	race->runners[1] = Harold;
	race->numRunners = 2;
	race->numDorcs = 0;
	sem_init(&race->mutex, 0, 1);
	race->winner[0] = '\0';
	race->statusRow = STATUS_ROW;

	srand((unsigned)time(NULL));

	initNcurses();
	scrPrt("Health: T  H", HEALTH_ROW, HEALTH_COL);

	pthread_create(&Timmy->ent.thr, NULL, goRunner, Timmy);
	pthread_create(&Harold->ent.thr, NULL, goRunner, Harold);

	int have_winner = 0;

	//Race loop
	while (1) {
		if (race->winner[0] != '\0') {
			have_winner = 1;
			break;
		}
		int ind = 0;
		for (int i = 0; i < race->numRunners; i++) {
			if (race->runners[i]->dead) {
				ind = 1;
				break;
			}
		}
		if (ind) {
			break;
		}
		int randNum = randm(10);
		if (randNum < DORC_FREQ) { //30% probability
			EntityType* newDorc = malloc(sizeof(EntityType));
			newDorc->avatar[0] = 'd';
			newDorc->avatar[1] = '\0';
			newDorc->currPos = 2;
			int randNum1 = randm(3);
			if (randNum1 == 0) { // 1/3 probability
				newDorc->path = PATH_1;
			}
			else if (randNum1 == 1) { // 1/3 probability
				newDorc->path = PATH_2;
			}
			else { // 1/3 probability
				newDorc->path = (PATH_1 + PATH_2) / 2;
			}	
			race->dorcs[race->numDorcs] = newDorc;
			race->numDorcs++;
			pthread_create(&newDorc->thr, NULL, goDorc, newDorc);
			usleep(250000);
		}
	}

	for (int i = 0; i < race->numRunners; i++) {
		pthread_join(race->runners[i]->ent.thr, NULL);		
	}

	for (int i = 0; i < race->numDorcs; i++) {
		pthread_cancel(race->dorcs[i]->thr);
	}

	scrPrt("OUTCOME: ", race->statusRow, STATUS_COL);
	if (have_winner) {
		scrPrt("The winner is ", race->statusRow, 39);
		if (race->winner[0] == 'T')
			scrPrt("Timmy", race->statusRow, 53);
		else
			scrPrt("Harold", race->statusRow, 53);
	}
	else {
		scrPrt("Both players died!", race->statusRow, 39);
	}

	cleanUp();

	return 0;
}