#include <stdio.h>
#include <time.h>
#include <stdlib.h>


struct PState {
    const unsigned int pHeight;
    const unsigned int pWidth;
    const unsigned int pSize;
    unsigned int pPlacedPieces;
    unsigned int pOpenEdges;
    double pTotalMoves;
    unsigned char* pState;  // State of the puzzle's placed pieces
    unsigned char* pEdges; // Open locations for pieces 
};

static unsigned int bigRand(unsigned int size)
{
    if (size <= 0x7fff)
    {
        return rand() % size;
    }
    else if (size <= 0xFFFE)
    {
        return ((rand() << 15) + rand()) % size;
    }
    else if (size <= 0x17FFD)
    {
        return ((rand() << 31) + (rand() << 15) + rand()) % size;
    }
    else
    {
        unsigned char rCount = size / 0xFFFE;
        unsigned long rVal = 0;
        for (unsigned char i = 0; i <= rCount; i++)
        {
            rVal += (rand() << (15 * i));
        }
        return rVal % size;
    }
}

static unsigned char* getStateByte(unsigned char* stateBits, unsigned int offset)
{
    return stateBits + offset / 8;
}

static unsigned char getValueByte(unsigned int offset)
{
    return 1 << (offset % 8);
}

static unsigned char isBitSet(unsigned char* stateByte, unsigned char setValue)
{
    return (*stateByte & setValue) != 0;
}

static unsigned char updateStateBit(unsigned char* stateByte, unsigned char setValue, unsigned char setBit)
{
    unsigned char isSet = isBitSet(stateByte, setValue);
    if (isSet && !setBit)
    {
        *stateByte -= setValue;
        return -1;
    }
    else if (!isSet && setBit)
    {
        *stateByte += setValue;
        return 1;
    }
    return 0;
}

static void setBit(unsigned char* stateBits, unsigned char offset)
{
    unsigned char* stateByte = getStateByte(stateBits, offset);
    unsigned char setValue = getValueByte(offset);
    updateStateBit(stateByte, setValue, 1);
}

static struct PState getPState(unsigned int pHeight, unsigned int pWidth)
{
    unsigned int pSize = pHeight * pWidth;
    struct PState pState = { pHeight, pWidth, pSize, 0, 0 };

    pState.pState = (unsigned char*)malloc(pSize);
    pState.pEdges = (unsigned char*)malloc(pSize);
    memset(pState.pState, 0, pSize);
    memset(pState.pEdges, 0, pSize);

    setBit(pState.pEdges, 0); // first corner
    setBit(pState.pEdges, pWidth - 1); // second corner
    setBit(pState.pEdges, pSize - pWidth); // third corner
    setBit(pState.pEdges, pSize - 1); // fourth corner
    pState.pOpenEdges = 4;

    return pState;
}

static void safeAddEdge(struct PState* pState, unsigned int offset)
{
    unsigned char* edgeByte = getStateByte(pState->pEdges, offset);
    unsigned char* stateByte = getStateByte(pState->pState, offset);
    unsigned char setValue = getValueByte(offset);
    if (!isBitSet(stateByte, setValue))  // Do not add edges already set in the state array
        pState->pOpenEdges += updateStateBit(edgeByte, setValue, 1); // returns 1 on opening new edge, 0 if edge already open
}

static void addNewEdges(struct PState* pState, unsigned int stepIdx)
{
    unsigned int pCol = stepIdx % pState->pWidth;
    if (stepIdx >= pState->pWidth)  // not first row
    {
        safeAddEdge(pState, stepIdx - pState->pWidth);
    }
    if (stepIdx < pState->pSize - pState->pWidth) // not last row
    {
        safeAddEdge(pState, stepIdx + pState->pWidth);
    }
    if (pCol != 0)  // Not left edge
    {
        safeAddEdge(pState, stepIdx - 1);
    }
    if (pCol != pState->pWidth - 1) // Not Right edge
    {
        safeAddEdge(pState, stepIdx + 1);
    }
}

static void stepSim(struct PState* pState)
{
    unsigned int piecesRemaining = pState->pSize - pState->pPlacedPieces;
    if (pState->pOpenEdges == piecesRemaining)
    {
        // If edges == pieces, then every piece is playable, each step will be 1 move and can be skipped
        pState->pOpenEdges = 0;
        pState->pTotalMoves += piecesRemaining;
        pState->pPlacedPieces += piecesRemaining;
        return;
    }
    unsigned char setValue = 0;
    unsigned char* edgeByte = NULL;
    unsigned char* stateByte = NULL;
    unsigned int stepIdx = 0;
    while (1)
    {
        stepIdx = bigRand(pState->pSize);
        setValue = getValueByte(stepIdx);
        stateByte = getStateByte(pState->pState, stepIdx);
        if (isBitSet(stateByte, setValue))
        {
            continue;
        }
        pState->pTotalMoves++;
        edgeByte = getStateByte(pState->pEdges, stepIdx);
        if (isBitSet(edgeByte, setValue))
        {
            break;
        }
    }

    updateStateBit(stateByte, setValue, 1);
    pState->pPlacedPieces++;

    updateStateBit(edgeByte, setValue, 0);
    pState->pOpenEdges--;

    addNewEdges(pState, stepIdx);
}


static struct PState runSim(unsigned int pWidth, unsigned int pHeight)
{
    struct PState pState = getPState(pWidth, pHeight);
    while (pState.pPlacedPieces < pState.pSize)
    {
        stepSim(&pState);
    }
    free(pState.pState);
    free(pState.pEdges);
    return pState;
}

static void doTestRun(unsigned int pStartSize, unsigned int pEndSize, unsigned int iterCount, char* outFilePathPrefix)
{
    for (unsigned int pX = pStartSize; pX <= pEndSize; pX++)
    {
        unsigned int size = pX * pX;
        char fileName[2048];
        sprintf(fileName, "%s_%d_%d.csv", outFilePathPrefix, iterCount, size);
        printf("Starting test run: %d, %dx%d, %s \n", iterCount, pX, pX, fileName);

        long double totalCount = 0;
        for (unsigned int i = 0; i < iterCount; i++) {
            struct PState pState = runSim(pX, pX);
            totalCount += pState.pTotalMoves;
        }
        float moveAvg = (float)totalCount / iterCount;
        FILE* pFile = fopen(fileName, "w");

        printf("%d, %d, %f\n", iterCount, size, moveAvg);
        if (pFile == NULL)
            printf("File open failed.");
        else
        {
            fprintf(pFile, "%d, %d, %f\n", iterCount, size, moveAvg);
            fclose(pFile);
        }
    }
}

static int main(int argc, char* argv[])
{
    unsigned int iterCount = 1;
    unsigned int pStartSize = 1;
    unsigned int pEndSize = 1;
    const char* outFilePathPrefix = "output";
    if (argc != 5)
    {
        printf("Usage  %s [iterCount] [pStartSize] [pEndSize] [outFilePathPrefix]\n", argv[0]);
        return -1;
    }
    else
    {
        iterCount = strtol(argv[1], (char**)NULL, 10);
        pStartSize = strtol(argv[2], (char**)NULL, 10);
        pEndSize = strtol(argv[3], (char**)NULL, 10);
        outFilePathPrefix = argv[4];
    }
    long t1 = time(NULL);
    srand(t1);
    doTestRun(pStartSize, pEndSize, iterCount, outFilePathPrefix);
}