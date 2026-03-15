#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 600

// temp config will add future :load map from file
// not always gridwidth = gridheight
#define GRID_WIDTH 28
#define GRID_HEIGHT 24
#define CELL_SIZE 24

#define GW GRID_WIDTH

// Best Practice: Static Inlines instead of Macros
static inline int GetIndex(int x, int y)
{
    return (y * GW) + x;
}

static inline int GetX(int index)
{
    return index % GW;
}

static inline int GetY(int index)
{
    return index / GW;
}

// Helper macro to convert (x,y) → index
/*
#define INDEX(x, y, w) ((y) * (w) + (x))
#define INDEXX(I, w) ((I) % (w))
#define INDEXY(I, w) ((I) / (w))
*/
typedef enum
{
    STATE_MENU,
    STATE_INGAME,
    STATE_PAUSED,
    STATE_GAMEOVER
} GameState;

typedef enum
{
    EMPTY = 0,
    SNAKE,
    SNAKEHEAD,
    SNAKETAIL,
    FOOD,
    ROCK
} CellType;

typedef struct
{
    Uint16 x;
    Uint16 y;
} Point; // Point pos;

typedef struct
{
    int index;
    CellType Type;
} Cell;

typedef struct
{
    Cell *cell;
    // SnakeNode* next;// SnakeNode* back;    int indx;
} SnakeNode;

typedef enum
{
    UP,
    DOWN,
    LEFT,
    RIGHT
} Direction;

typedef struct
{
    int length;
    int maxlength;
    int front;
    Direction current_direction;
    Direction next_direction;

    SnakeNode *Body; // queue
    // Cell *head; Cell *tail;  int rear;
} Snake;

typedef struct
{
    int gridwidth, gridheight;
    float cellsize;
    Cell *grid;
    int gridemptysize;
    Cell *food;
    Snake snake;
    int gridsize;
} GameWorld;

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *bg_texture;
    GameState state;
    GameState prev_state; // Useful for returning from Pause to InGame
    GameWorld game;
    Uint64 last_ticks;
} AppState;

void GeneriteFood(AppState *as)
{
    // for (int i = 0; i < as->game.gridsize; i++){}
    /*
        as->game.gridempty[i].Type = FOOD;
        as->game.food[0] = as->game.gridempty[i];
    */
    while (true)
    {
        int i = SDL_rand(as->game.gridemptysize);
        if (as->game.grid[i].Type == EMPTY)
        {
            as->game.grid[i].Type = FOOD;
            as->game.food[0] = as->game.grid[i];
            break;
        }
    }
    // as->game.gridemptysize--;

    return;
}

void LoadWorldMap(AppState *as)
{
    GameWorld *gw = &as->game;

    gw->gridwidth = GRID_WIDTH;
    gw->gridheight = GRID_HEIGHT;

    gw->cellsize = CELL_SIZE; // WINDOW_HEIGHT / gw->gridheight;

    gw->gridsize = gw->gridwidth * gw->gridheight;
    gw->grid = SDL_calloc(gw->gridsize, sizeof(Cell));
    if (gw->grid == NULL)
        SDL_Log("SDL_calloc : gw->snake.Body");
    gw->food = SDL_calloc(1, sizeof(Cell));
    if (gw->food == NULL)
        SDL_Log("SDL_calloc : gw->snake.Body");

    int gridempty = 0;
    for (int i = 0; i < gw->gridsize; i++)
    {
        int x = GetX(i);
        int y = GetY(i);
        gw->grid[i].index = i;
        if ((x == 0) || (x == gw->gridwidth - 1) || (y == 0) || (y == gw->gridheight - 1))
        {
            gw->grid[i].Type = ROCK;
        }
        else
        {
            gw->grid[i].Type = EMPTY;
            gridempty++;
        }
    }
    gw->gridemptysize = gridempty;
}

int SnakeRear(GameWorld *gw)
{
    int rear = (gw->snake.front + gw->snake.length - 1) % gw->snake.maxlength;
    return rear;
}

int SnakeNextRear(GameWorld *gw)
{
    int rear = (gw->snake.front + gw->snake.length) % gw->snake.maxlength;
    return rear;
}

Cell *SnakeHead(GameWorld *gw)
{
    int rear = SnakeRear(gw);
    return gw->snake.Body[rear].cell;
}

void SnakeRemoveRear(GameWorld *gw) // dequeue
{
    gw->snake.Body[gw->snake.front].cell->Type = EMPTY;
    gw->snake.front = (gw->snake.front + 1) % gw->snake.maxlength;
    gw->snake.length--;
}

void SnakeNewHead(GameWorld *gw, Cell *newhead) // enqueue
{
    int rear = SnakeNextRear(gw);
    SnakeNode *node = &gw->snake.Body[rear];
    node->cell = newhead;
    node->cell->Type = SNAKEHEAD;
    gw->snake.length++;
}

void SnakeInit(AppState *as)
{
    GameWorld *gw = &as->game;
    gw->snake.maxlength = gw->gridemptysize;

    gw->snake.Body = SDL_calloc(gw->snake.maxlength, sizeof(SnakeNode));
    if (gw->snake.Body == NULL)
        SDL_Log("SDL_calloc : gw->snake.Body");

    gw->snake.front = 0; // gw->snake.rear = -1;
    gw->snake.length = 0;

    SnakeNewHead(gw, &as->game.grid[GRID_WIDTH + 1]);

    as->game.snake.current_direction = SDL_rand(sizeof(Direction));
    as->game.snake.next_direction = as->game.snake.current_direction;
}

void SnakeLogic(AppState *as)
{
    GameWorld *gw = &as->game;
    SDL_Point next_pos;
    Cell *head = SnakeHead(gw);

    next_pos.x = GetX(head->index);
    next_pos.y = GetY(head->index);

    gw->snake.current_direction = gw->snake.next_direction;
    switch (gw->snake.current_direction)
    {
    case UP:
        next_pos.y--;
        break;
    case DOWN:
        next_pos.y++;
        break;
    case LEFT:
        next_pos.x--;
        break;
    case RIGHT:
        next_pos.x++;
        break;
    }

    Cell *next_cell = &gw->grid[GetIndex(next_pos.x, next_pos.y)];
    switch (next_cell->Type)
    {
    case EMPTY:
        Cell *head = SnakeHead(gw);
        head->Type = SNAKE;
        SnakeNewHead(gw, &gw->grid[next_cell->index]);
        SnakeRemoveRear(gw);

        break;
    case FOOD:
        SnakeNewHead(gw, &gw->grid[next_cell->index]);
        if (gw->snake.length == gw->gridemptysize)
            SDL_Log("Game End.");
        else
            GeneriteFood(as);

        break;
    default:
        as->game.snake.next_direction = SDL_rand(sizeof(Direction)); // will add game over future.. .
        break;
    }
}

void CreateBackground(AppState *as)
{
    GameWorld *gw = &as->game;
    int pixel_w, pixel_h;
    SDL_GetRenderOutputSize(as->renderer, &pixel_w, &pixel_h);
    as->bg_texture = SDL_CreateTexture(as->renderer,
                                       SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 
                                       pixel_w, pixel_h);

    SDL_SetRenderTarget(as->renderer, as->bg_texture);

    SDL_SetRenderTarget(as->renderer, as->bg_texture);

    SDL_SetRenderDrawColor(as->renderer, 150, 200, 200, 255); // backgroung color
    SDL_RenderClear(as->renderer);

    SDL_SetRenderDrawColor(as->renderer, 144, 144, 144, 255); // grid lines
    for (int i = 0; i <= gw->gridwidth; i++)
    {
        SDL_RenderLine(as->renderer, i * gw->cellsize, 0, i * gw->cellsize, gw->gridheight * gw->cellsize);
    }

    for (int i = 0; i <= gw->gridheight; i++)
    {
        SDL_RenderLine(as->renderer, 0, i * gw->cellsize, gw->gridwidth * gw->cellsize, i * gw->cellsize);
    }

    SDL_SetRenderDrawColor(as->renderer, 88, 88, 88, 255); // rock
    for (int i = 0; i < as->game.gridsize; i++)
    {
        if (as->game.grid[i].Type == ROCK)
        {
            SDL_FRect rect;
            rect.x = GetX(i) * as->game.cellsize + 2;
            rect.y = GetY(i) * as->game.cellsize + 2; // as->game.grid[j].pos.y
            rect.h = as->game.cellsize - 3;
            rect.w = as->game.cellsize - 3;
            SDL_RenderFillRect(as->renderer, &rect);
        }
    }

    SDL_SetRenderTarget(as->renderer, NULL);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    AppState *as = (AppState *)SDL_calloc(1, sizeof(AppState));
    if (!as)
    {
        return SDL_APP_FAILURE;
    }
    *appstate = as;
    if (!SDL_CreateWindowAndRenderer("snake02", WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_HIGH_PIXEL_DENSITY, &as->window, &as->renderer))
    {
        return SDL_APP_FAILURE;
    }

    as->state = as->prev_state = STATE_INGAME;

    as->last_ticks = SDL_GetTicks();

    LoadWorldMap(as);
    GeneriteFood(as);
    SnakeInit(as);
    CreateBackground(as);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *as = (AppState *)appstate;
    Snake *snk = &as->game.snake;
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    /*  case SDL_EVENT_WINDOW_FOCUS_GAINED:
            as->state = STATE_INGAME;
            break;
        case SDL_EVENT_WINDOW_FOCUS_LOST:
            if (as->state == STATE_INGAME)
                as->state = STATE_PAUSED;
            break;*/
    case SDL_EVENT_KEY_DOWN:
        switch (event->key.key)
        {
        case SDLK_ESCAPE:
        case SDLK_P:
            if (as->state == STATE_INGAME)
                as->state = STATE_PAUSED;
            else if (as->state == STATE_PAUSED)
                as->state = STATE_INGAME;
            break;
        case SDLK_UP:
            if (snk->current_direction != DOWN)
                snk->next_direction = UP;
            break;
        case SDLK_DOWN:
            if (snk->current_direction != UP)
                snk->next_direction = DOWN;
            break;
        case SDLK_LEFT:
            if (snk->current_direction != RIGHT)
                snk->next_direction = LEFT;
            break;
        case SDLK_RIGHT:
            if (snk->current_direction != LEFT)
                snk->next_direction = RIGHT;
            break;
        }
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

// void RenderSnake(void *appstate){}

void RenderGame(void *appstate)
{
    AppState *as = (AppState *)appstate;
    GameWorld *gw = &as->game;

    // Clear the real backbuffer
    SDL_SetRenderDrawColor(as->renderer, 0, 0, 0, 255);
    SDL_RenderClear(as->renderer);

    SDL_RenderTexture(as->renderer, as->bg_texture, NULL, NULL); // render background
    // render food
    SDL_SetRenderDrawColor(as->renderer, 0, 150, 0, 255);
    SDL_FRect rect;

    rect.x = GetX(gw->food->index) * gw->cellsize + 2;
    rect.y = GetY(gw->food->index) * gw->cellsize + 2; // as->game.grid[j].pos.y
    rect.h = gw->cellsize - 3;
    rect.w = gw->cellsize - 3;
    SDL_RenderFillRect(as->renderer, &rect);

    // render snake
    SDL_SetRenderDrawColor(as->renderer, 150, 10, 10, 255);
    for (int i = 0; i < gw->snake.length; i++)
    {
        int indx = (i + gw->snake.front) % gw->snake.maxlength;
        rect.x = GetX(gw->snake.Body[indx].cell->index) * gw->cellsize + 2;
        rect.y = GetY(gw->snake.Body[indx].cell->index) * gw->cellsize + 2;
        rect.h = gw->cellsize - 3;
        rect.w = gw->cellsize - 3;
        SDL_RenderFillRect(as->renderer, &rect);
    }

    SDL_SetRenderDrawColor(as->renderer, 120, 100, 150, 255);
    rect.x = GetX(SnakeHead(gw)->index) * as->game.cellsize + 2;
    rect.y = GetY(SnakeHead(gw)->index) * as->game.cellsize + 2;
    rect.h = as->game.cellsize - 3;
    rect.w = as->game.cellsize - 3;
    SDL_RenderFillRect(as->renderer, &rect);

    SDL_RenderPresent(as->renderer);
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *as = (AppState *)appstate;

    Uint64 now = SDL_GetTicks();
    int delta_time = (now - as->last_ticks);

    if (delta_time < 30)
    {
        return SDL_APP_CONTINUE;
    }
    as->last_ticks = now;

    switch (as->state)
    {
    case STATE_MENU:
        // UpdateMenu();
        // RenderMenu(as->renderer);
        break;

    case STATE_INGAME:
        SnakeLogic(as);
        RenderGame(as);
        break;

    case STATE_PAUSED:
        // We don't call UpdateGameLogic() here, so time "stops"
        // RenderGame(as->renderer);
        // RenderPauseOverlay(as->renderer); // Dim screen + "PAUSED" text
        break;

    case STATE_GAMEOVER:
        // RenderGame(as->renderer);
        // RenderGameOverScreen(as->renderer);
        break;

    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    AppState *as = (AppState *)appstate;
    if (as)
    {
        SDL_DestroyTexture(as->bg_texture);
        SDL_free(as->game.food);
        SDL_free(as->game.snake.Body);
        // SDL_free(as->game.gridempty);
        SDL_free(as->game.grid);
        SDL_free(as);
    }
    if (result == SDL_APP_FAILURE)
    {
        SDL_Log("SDL_AppQuit with err");
    }
}
