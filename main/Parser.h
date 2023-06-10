#include <fstream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <stdio.h>


// to supply large file ------
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
// ---------------------------

constexpr long long MAXS = 1800*1024*1024;  // 2047MB
char buf[MAXS];
constexpr int SIZE_OF_NAME = 1028;


struct MultiNet
{
    const std::string name;
    const int ID;
    MultiNet(std::string name, int id) : name(name), ID(id) {}
    ~MultiNet() {}
};


struct Cell
{
    const int ID;
    const std::string name;
    const std::string type;
    std::vector <std::string> SignleNets;
    std::vector <MultiNet> MultiNets;

    Cell(char* name, char* type, int id) : 
        name(name), type(type), ID(id) {}
    ~Cell() {}                                                                                                                                   
};

struct Net
{
    const int ID;
    const std::string name;
    const int bitNum;
    //std::vector <Cell*> Cells;
    std::set <Cell*> Cells;

    Net(std::string name, int id, int bitNum) : name(name), ID(id), bitNum(bitNum) {}
    ~Net() 
    {
        Cells.clear();
        //Cells.shrink_to_fit();
    }
};


class Parser
{
private:
    void ReadFile(std::string fileName);
    bool GetSegment();
    void GetNets(size_t& ptr);
    void GetCells(size_t& ptr);
    bool isCharacter(char& cc);
    bool isStr(const char* str, size_t& ptr);
    void SkipSpace(size_t& ptr);
    int str2int(char* str);
    void GetNetinCell(size_t& ptr, std::vector <std::string>& Single_Nets_in_Cell, std::vector <MultiNet>& Multi_Nets_in_Cell);
    void PutCellintoNet(Cell* cell);
    void WriteHgr();
    void WriteCell2CodeDecoder();
    void GetNetNum();
    void HandleCombinedNets(size_t& ptr, std::vector <std::string>& Single_Nets_in_Cell, std::vector <MultiNet>& Multi_Nets_in_Cell);
    //std::vector <Net*>& FindSingleNetVector(std::string str);
    //std::vector <Net*>& FindMultiNetVector(std::string str);

    size_t ptr_of_buf = 0;
    size_t len_of_buf = 0;
    size_t len_of_seg = 0;

    int cellNum = 0;
    int netNum = 0;

    char segment[1024 * 1024] = {'\0'};
    //std::vector <Net*> SingleNets;
    std::map< std::string, Net* > SingleNets;
    //std::vector <Net*> MultiNets;
    std::map< std::string, std::vector <Net*> > MultiNets;  // netname : net[31], net[30], ...
    std::vector <Cell*> Cells;

public:
    Parser(std::string fileDir);
    ~Parser();

    void Parse(std::string fileName);
    
    void ShowCells();
    void ShowNets();

    std::string workDir;
    std::string inputFileName;
    std::string inputFileDir;
    std::string outputHgrFileName;
    std::string outputCell2CodeCsvName;
    std::string showCellsFileName;
    std::string showNetsFileName;
    std::string errorFileName;

    FILE* errorLogFile;
};

Parser::Parser(std::string fileDir)
{
    this->inputFileDir = fileDir;

    size_t i = fileDir.size() - 1;
    while ((fileDir.at(i) != '/') && (fileDir.at(i) != '\\'))
        --i;
    this->workDir = fileDir.substr(0, i + 1);
    this->inputFileName = fileDir.substr(i + 1, fileDir.size() - i + 1);

    std::string fileName = this->inputFileName;
    while (fileName.at(fileName.size()-1) != '.')
        fileName.pop_back();
    fileName.pop_back();

    this->outputHgrFileName = this->workDir + fileName + ".hgr";
    this->outputCell2CodeCsvName = this->workDir + fileName + "_Cell2Code" + ".csv";
    this->showCellsFileName = this->workDir + fileName + "_ShowCells.log";
    this->showNetsFileName = this->workDir + fileName + "_ShowNets.log";
    this->errorFileName = this->workDir + fileName + "_error.log";
    this->errorLogFile = fopen(this->errorFileName.c_str(), "w");
}

Parser::~Parser()
{
}
