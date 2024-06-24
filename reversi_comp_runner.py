# Reversi AI

import copy
import importlib
import math
import pygame as pg
import random
from reversi_common import *
import time

# Dynamically import competition AI

player1 = importlib.import_module("player_1")
player2 = importlib.import_module("player_2")

board = [[E, E, E, E, E, E, E, E],
         [E, E, E, E, E, E, E, E],
         [E, E, E, E, E, E, E, E],
         [E, E, E, W, B, E, E, E],
         [E, E, E, B, W, E, E, E],
         [E, E, E, E, E, E, E, E],
         [E, E, E, E, E, E, E, E],
         [E, E, E, E, E, E, E, E]]


# ------------------
# Pygame visual code
# ------------------

# Setup pygame and game objects
SCREEN_WIDTH = 900
SCREEN_HEIGHT = 600
SQUARE_WIDTH = 75
BOARD_WIDTH = 8*SQUARE_WIDTH
pg.init()
clock = pg.time.Clock()
screen = pg.display.set_mode((SCREEN_WIDTH, SCREEN_HEIGHT), pg.SCALED)
background = pg.Surface(screen.get_size())
font = pg.font.SysFont("Courier", 22)

class Label:

    labels = []

    def __init__(self, x, y, text):
        self.x = x
        self.y = y
        self.text = text
        Label.labels.append(self)

    def set_text(self, text):
        self.text = text

    def render(self):
        text = font.render(self.text, True, "black")
        text_rect = text.get_rect().move(self.x, self.y)
        screen.blit(text, text_rect)

    @staticmethod
    def render_all():
        for label in Label.labels:
            label.render()

class Button:

    buttons = []

    def __init__(self, x, y, width, height, text, action):
        self.rect = pg.Rect(x, y, width, height)
        self.text = text
        self.action = action
        Button.buttons.append(self)

    def render(self):
        pg.draw.rect(screen, "white", self.rect, 3, 10)
        text = font.render(self.text, True, "black")
        text_rect = text.get_rect()
        text_rect.center = self.rect.center
        screen.blit(text, text_rect)

    def click(self):
        self.action(self)

    @staticmethod
    def check_click(point):
        for button in Button.buttons:
            if button.rect.collidepoint(point):
                button.action()

    @staticmethod
    def render_all():
        for button in Button.buttons:
            button.render()
        

current_player = B

# To change AIs, set these variables to the name of a function
# that takes two arguments: a board and the current player.

# The function should return a dictionary containing a "moves"
# key, whose value is a list of pairs representing moves.

white_ai = player1.ai_moves
black_ai = player2.ai_moves

b_time = 60
w_time = 60

def human_turn():
    return ((current_player == B and black_ai == None) or
            (current_player == W and white_ai == None))

def update():
    global current_player
    global b_time
    global w_time

    if not paused:
        turn_start = time.perf_counter()
        moves = []
        if current_player == B and black_ai:
            moves = black_ai(board, B, b_time)["moves"]
        elif current_player == W and white_ai:
            moves = white_ai(board, W, w_time)["moves"]

        if len(moves) > 0:
            move = random.choice(moves)
            if not play(board, current_player, move[0], move[1]):
                print("INVALID MOVE??")
                print(current_player)
            turn_end = time.perf_counter()
            if current_player == B:
                current_player = W
                b_time -= (turn_end-turn_start)
            else:
                current_player = B
                w_time -= (turn_end - turn_start)
            toplay_label.set_text("To play: " + current_player)
        score_label.set_text("Score: " + str(evaluate(board)))
        b_time_label.set_text("Black time: " + f'{b_time:.2f}')
        w_time_label.set_text("White time: " + f'{w_time:.2f}')

def render():
    for i in range(9):
        pg.draw.line(screen, B, (0, i*SQUARE_WIDTH), (BOARD_WIDTH, i*SQUARE_WIDTH), 2)
        pg.draw.line(screen, B, (i*SQUARE_WIDTH, 0), (i*SQUARE_WIDTH, BOARD_WIDTH), 2)

    for y in range(8):
        for x in range(8):
            pg.draw.circle(screen, board[y][x], ((x+0.5)*SQUARE_WIDTH, (y+0.5)*SQUARE_WIDTH), SQUARE_WIDTH/2 - 5)


toplay_label = Label(650, 50, "To play: " + current_player)
score_label = Label(650, 80, "Score: " + str(evaluate(board)))
b_time_label = Label(650, 200, "Black time: " + str(b_time))
w_time_label = Label(650, 250, "White time: " + str(w_time))
paused_label = Label(650, 500, "")

# Start game loop
running = True
paused = False
while running:
    # poll for events
    # pygame.QUIT event means the user clicked X to close your window
    for event in pg.event.get():
        if event.type == pg.QUIT:
            running = False
        if event.type == pg.MOUSEBUTTONDOWN and human_turn():
            Button.check_click(event.pos)
            x, y = event.pos
            if x < BOARD_WIDTH and y < BOARD_WIDTH:
                col = x // 75
                row = y // 75
                if play(board, current_player, col, row):
                    if current_player == B:
                        current_player = W
                    else:
                        current_player = B
                    toplay_label.set_text("To play: " + current_player)
                    render()
        if event.type == pg.KEYDOWN and event.key == pg.K_p:
            paused = not paused
            if paused:
                paused_label.set_text("Paused")
            else:
                paused_label.set_text("")
            
    # fill the screen with a color to wipe away anything from last frame
    screen.fill(E)
    
    # UPDATE YOUR GAME HERE
    update()

    render()

    Label.render_all()
    Button.render_all()

    # flip() the display to update your work on screen
    pg.display.flip()

    clock.tick(60) # limits FPS to 60

# Quit the game and do any necessary shutdown actions
pg.quit()

                
    
    
