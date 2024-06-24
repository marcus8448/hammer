E = (17, 102, 0)
B = "Black"
W = "White"

def display(board):
    """Prints out a formatted version of board."""
    print("-----------------")
    for row in board:
        row_string = "|"
        for space in row:
            row_string += space + "|" 
        print(row_string)
        print("-----------------")

def on_board(x, y):
    """Returns True if the position at (x, y) is on the board."""
    return 0 <= x <= 7 and 0 <= y <= 7

def next_player(player):
    """Returns the piece of the next player to play)"""
    if player == B:
        return W
    else:
        return B

def pieces_to_flip(board, piece, x, y):
    """Returns list of piece coordinates to be flipped if the given piece is
       played at (x, y)."""
    if board[y][x] != E:
        return []
    pieces_to_flip = []
    # 8 directions to check for flipping other player's pieces
    directions = [(1, 0), (1, 1), (0, 1), (-1, 1), (-1, 0), (-1, -1), (0, -1), (1, -1)]
    for i, j in directions:
        x_now = x+i
        y_now = y+j
        pieces_to_add = []
        # Continue in direction while you encounter other player's pieces
        while on_board(x_now, y_now) and board[y_now][x_now] not in [piece, E]:
            pieces_to_add.append((x_now, y_now))
            x_now += i
            y_now += j
        # If next piece is your own, flip all pieces in between
        if on_board(x_now, y_now) and board[y_now][x_now] == piece:
            pieces_to_flip += pieces_to_add
    return pieces_to_flip

def legal_moves(board, piece):
    """Returns list of coordinates of legal moves for the given piece."""
    moves = []
    for i in range(len(board)):
        for j in range(len(board[i])):
            if pieces_to_flip(board, piece, j, i) != []:
                moves.append((j, i))
    return moves

def play(board, piece, x, y):
    """If playing at (x, y) is legal, plays piece there and returns True.
       Otherwise leaves board unchanged and returns False."""
    flipped = pieces_to_flip(board, piece, x, y)
    # Move is legal if any pieces are flipped
    if flipped != []:
        board[y][x] = piece
        for i, j in flipped:
            board[j][i] = piece
        return True
    else:
        return False

def evaluate(board):
    """
    Returns the evaluation score of the board.
    A score of zero indicates a neutral position.
    A positive score indicates that black is winning.
    A negative score indicates that white is winning.
    """
    score = 0
    for y in range(8):
        for x in range(8):
            if board[y][x] == B:
                score += 1
            elif board[y][x] == W:
                score -= 1
    return score
