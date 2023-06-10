#include "Parser.cpp"
#include <time.h>
#include <chrono>

  
#define TIME_START              \
    clock_t ____start, ____end; \
    ____start = clock()
#define TIME_OUT                \
    ____end = clock();          \
    printf("RUNNING TIME = %.2fs\n", double(____end - ____start)/1000000.0)


int main(int argc, char* argv[])
{
    TIME_START;

    std::string fileDir;
    if (argc == 1)
    {
        fileDir = "./work/carpat/carpat_stratixiv.vqm";
    }
    else if (argc == 2)
    {
        fileDir = argv[1];
    }
    else
    {
        printf("ERROR! INPUT TOO MANY ARGS!\n");
        return 1;
    }

    Parser parser(fileDir);
    parser.Parse(parser.inputFileDir);
    parser.ShowCells();
    parser.ShowNets();

    TIME_OUT;

    fprintf(parser.errorLogFile, "RUNNING TIME = %.2f s\n", double(____end - ____start)/1000000.0);
    auto now = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(now);
    fprintf(parser.errorLogFile, "Parse Time : %s", std::ctime(&end_time));
    printf("Write in %s Success !\n", parser.errorFileName.c_str());

    fclose(parser.errorLogFile);

    return 0;
}