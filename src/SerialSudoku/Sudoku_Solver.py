import json
import time
import numpy as np

def backtrackingSearch(csp, assignment):
    domains = findDomains(assignment)
    return backtrack(csp, assignment, domains)

def backtrack(csp, assignment, domains):
    if isComplete(assignment):
        return assignment
    var = selectUnassignedVariableMRV(csp, assignment, domains)
    for value in orderDomainValuesLCV(csp, var, assignment, domains):
        if isConsistent(var, value, assignment):
            assignment[var[0]][var[1]] = value
            holdDomain = domains[var[0]][var[1]]
            domains[var[0]][var[1]] = [value]
            inferences = inference(csp, var, assignment, domains)
            if inferences[1]:
                result = backtrack(csp, assignment, domains)
                if result != [["No solution"]]:
                    return result
            removeInferences(inferences, domains, value)
            assignment[var[0]][var[1]] = 0
            domains[var[0]][var[1]] = holdDomain
    return [["No solution"]]

def isComplete(assignment):
    for row in assignment:
        for col in row:
            if col == 0:
                return False
    return True

def selectUnassignedVariable(csp, assignment, domains):
    for i in range(len(assignment)):
        for j in range(len(assignment[i])):
            if assignment[i][j] == 0:
                #print("This one:", i, j)
                return i, j

def selectUnassignedVariableMRV(csp, assignment, domains):
    smallestVar = [] 
    smallestDomain = len(assignment) + 1
    for i in range(len(assignment)):
        for j in range(len(assignment[i])):
            if assignment[i][j] == 0:
                #domain = findDomain((i, j), assignment)
                domain = domains[i][j]
                if len(domain) < smallestDomain:
                    smallestVar = (i, j)
                    smallestDomain = len(domain)
    return smallestVar
            
def orderDomainValues(csp, var, assignment, domains):
    return domains[var[0]][var[1]]
    #return list(range(1, len(assignment) + 1))

def orderDomainValuesLCV(csp, var, assignment, domains):
    values = domains[var[0]][var[1]]
    #values = findDomain(var, assignment)
    #print(values)
    constraining = []
    for value in values:
        count = 0
        for neighbor in getNeighbors(var, assignment):
            if assignment[neighbor[0]][neighbor[1]] == 0:
                #for val in findDomain(neighbor, assignment):
                for val in domains[neighbor[0]][neighbor[1]]:
                    if val == value:
                        count += 1
        constraining.append((count, value))
    constraining.sort()
    orderedValues = [pair[1] for pair in constraining]
    #print (orderedValues)
    return orderedValues

def getNeighbors(var, assignment):
    neighbors = []
    #Get row neighbors
    for j in range(len(assignment)):
        if j != var[1]:
            neighbors.append((var[0], j))
    #Get column neighbors
    for i in range(len(assignment)):
        if i != var[0]:
            neighbors.append((i, var[1]))
    #Get box neighbors
    num = (int)(len(assignment) ** 0.5)
    xBox = var[0] // num
    yBox = var[1] // num
    for i in range(num):
        for j in range(num):
            if xBox * num + i != var[0] and yBox * num + j != var[1]: #No repeats!
                neighbors.append((xBox * num + i, yBox * num + j))
    return neighbors

def isConsistent(var, value, assignment):
    #check rows
    for point in assignment[var[0]]:
        if point == value:
            #print("Row Problem")
            return False
    #check columns
    for i in range(len(assignment)):
        if assignment[i][var[1]] == value:
            #print("Column Problem")
            return False
    #check boxs
    num = (int)(len(assignment) ** 0.5)
    xBox = var[0] // num
    yBox = var[1] // num
    for i in range(num):
        for j in range(num):
            if assignment[xBox * num + i][yBox * num + j] == value:
                #print("Box Problem")
                return False
    return True

def printData(assignment):
    for row in assignment:
        print(row)

def findDomain(var, assignment):
    domain = []
    #print("I'm here!")
    if(assignment[var[0]][var[1]] != 0):
        return [assignment[var[0]][var[1]]]
    for i in range(1, len(assignment) + 1):
        if isConsistent(var, i, assignment):
            domain.append(i)
    #print("This is the domain", domain, "for variable", var)
    return domain

def findDomains(assignment):
    domains = [[0 for point in range(len(assignment))] for row in range(len(assignment))]
    for i in range(len(assignment)):
        for j in range(len(assignment[i])):
            domains[i][j] = findDomain((i, j), assignment)
    #print(domains)
    return domains

def removeInferences(inferences, domains, value):
    for inference in inferences[0]:
        domains[inference[0][0]][inference[0][1]].append(inference[1])
    return True

def inference(csp, var, assignment, domains):
    inferences = []
    for neighbor in getNeighbors(var, assignment):
        try:
            i = domains[neighbor[0]][neighbor[1]].index(assignment[var[0]][var[1]])
            domains[neighbor[0]][neighbor[1]].pop(i)
            inferences.append((neighbor, assignment[var[0]][var[1]])) #(var, value removed)
            if len(domains[neighbor[0]][neighbor[1]]) == 0:
                return (inferences, False)
        except ValueError:
            continue
    return (inferences, True)

def inferenceAC3(csp, var, assignment, domains):
    inferences = []
    queue = []
    for neighbor in getNeighbors(var, assignment):
        try: #Forward checking
            i = domains[neighbor[0]][neighbor[1]].index(assignment[var[0]][var[1]])
            domains[neighbor[0]][neighbor[1]].pop(i)
            inferences.append((neighbor, assignment[var[0]][var[1]])) #(var, value removed)
            if len(domains[neighbor[0]][neighbor[1]]) == 0:
                return (inferences, False)
        except ValueError:
            pass
        if assignment[neighbor[0]][neighbor[1]] == 0:
            queue.append((neighbor, var))
    while len(queue) != 0:
        xi, xj = queue.pop(0)
        if revise(csp, xi, xj, domains, inferences):
            if len(domains[xi[0]][xi[1]]) == 0:
                return (inferences, False)
            for neighbor in getNeighbors(xi, assignment):
                if assignment[neighbor[0]][neighbor[1]] == 0 and neighbor != xj:
                    queue.append((neighbor, xi))
    return (inferences, True)

def revise(csp, xi, xj, domains, inferences):
    revised = False
    i = 0
    while i < len(domains[xi[0]][xi[1]]):
        flag = False
        for jValue in domains[xj[0]][xj[1]]:
            if domains[xi[0]][xi[1]][i] != jValue:
                flag = True
                break
        if not flag:
            inferences.append((xi, domains[xi[0]][xi[1]][i])) #Inferences are in this format (var, value removed)
            domains[xi[0]][xi[1]].pop(i)
            i -= 1
            revised = True
        i += 1
    return revised
    

#----------------------------------

with open('sudoku5_300_1.json', 'r') as f:
    board = json.load(f)

printData(board)
print("The answer is:")
start = time.perf_counter()
printData(backtrackingSearch(0, board))
end = time.perf_counter()
print("This took", (end - start), "seconds")

# Note: I have 3 different heuristics implemented: Minimum remaining value, Least constraining value, and Maintaining arc consistency.
# These heuristics can be included or not by changing the function calls in the "backtrack" function to their base forms

# selectUnassignedVariable (base) vs selectUnassignedVariableMRV (heuristic) - MRV is very useful
# orderDomainValues (base) vs orderDomainValuesLCV (heuristic) - LCV is only useful when MRV is included as well
# inference (base) vs inferenceAC3 (heuristic) - AC3 did not help. Maybe it could be rewritten to be better?

# Also, to test my program, I created quite a few sudoku puzzles with the included SudokuGenerator (which was provided by the professor).
# I don't know if these would be at all useful, but I figure too much data can't hurt :)

# The naming scheme of the puzzles is: 
# "sudoku" + block size (3 for 9x9, 4 for 16x16) + "_" + number of empty (0) squares + "_" + an index for duplicate spec puzzles + ".json"