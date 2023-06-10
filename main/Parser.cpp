#include "Parser.h"
#include <iostream>
#include <string.h>
#include <regex>


std::vector<std::string> SplitBySpace(std::string str)
{
    std::string buf;
    std::stringstream ss(str);

    std::vector<std::string> tokens;

    while (ss >> buf)
        tokens.emplace_back(buf);

    return tokens;
}


void Parser::ReadFile(std::string fileName)
{
    std::ifstream netlist_file_(fileName, std::ios::binary);
    netlist_file_.seekg(0, netlist_file_.end);
    len_of_buf = (long long)netlist_file_.tellg();
    netlist_file_.seekg(0, std::ios::beg).read(&buf[0], len_of_buf);
    netlist_file_.close();

    buf[len_of_buf] = '\0';
}


bool Parser::GetSegment()
{
    if (ptr_of_buf >= len_of_buf)
    {
        return false;  // EOF
    }

    len_of_seg = 0;
    while (buf[ptr_of_buf] != ';')
    {
        if ((buf[ptr_of_buf] == '/') && (buf[ptr_of_buf+1] == '/'))  // skip the line of explanatory note
        {
            while (buf[ptr_of_buf] != '\n')
            {
                ++ptr_of_buf;
            }
            ++ptr_of_buf;
            continue;
        }
        segment[len_of_seg++] = buf[ptr_of_buf++];

        if (ptr_of_buf >= len_of_buf)
        {
            return false;  // EOF
        }
    }
    ++ptr_of_buf;

    segment[len_of_seg] = '\0';

    return true;
}


int Parser::str2int(char* str)
{
    int ret = 0;
    bool positiveNum = str[0] == '-' ? false : true;
    if (positiveNum)
    {
        for (size_t i = 0; i < strlen(str); ++i)
        {
            ret = ret * 10 + str[i] - '0';
        }
    }
    else
    {
        for (size_t i = 1; i < strlen(str); ++i)
        {
            ret = ret * 10 + str[i] - '0';
        }
        ret = 0 - ret;
    }
    return ret;
}


void Parser::GetNets(size_t& ptr)
{
    char name[SIZE_OF_NAME] = {'\0'};
    size_t i = 0;
    SkipSpace(ptr);
    if (segment[ptr] == '[')
    {
        ++ptr;
        char highBit_str[8] = {'\0'};
        char lowBit_str[8] = {'\0'};

        while (segment[ptr] != ':')
        {
            highBit_str[i++] = segment[ptr++];
        }
        highBit_str[i] = '\0'; i = 0; ++ptr;
        while (segment[ptr] != ']')
        {
            lowBit_str[i++] = segment[ptr++];
        }
        lowBit_str[i] = '\0'; i = 0; ++ptr;
        
        int highBit = str2int(highBit_str);
        int lowBit = str2int(lowBit_str);

        if (highBit < lowBit)  // swap if high < low
        {
            int temp = lowBit;
            lowBit = highBit;
            highBit = temp;
        }

        SkipSpace(ptr);
        while (ptr < len_of_seg)
        {
            if (segment[ptr] != ' ')
            {
                name[i++] = segment[ptr++];
            }
            else
            {
                break;
            }
        }

        int bitNum = highBit - lowBit + 1;
        
        std::vector <Net*> multi_nets_vec;
        for (int j = highBit; j >= lowBit; --j)
        {
            Net* net = new Net(name, j, bitNum);
            multi_nets_vec.emplace_back(net);
        }
        MultiNets.insert(std::pair<std::string, std::vector <Net*>>(name, multi_nets_vec));

    }
    else
    {
        while (ptr < len_of_seg)
        {
            if (segment[ptr] != ' ')
            {
                name[i++] = segment[ptr++];
            }
            else
            {
                break;
            }
        }
        Net* net = new Net(name, 0, 1);
        //SingleNets.emplace_back(net);
        SingleNets.insert(std::pair<std::string, Net*>(name, net));
    }
}


void Parser::GetCells(size_t& ptr)
{
    char name[SIZE_OF_NAME] = {'\0'};
    char type[SIZE_OF_NAME] = {'\0'};
    size_t i = 0;
    SkipSpace(ptr);
    while (isCharacter(segment[ptr]))
    {
        type[i++] = segment[ptr++];
    }
    type[i] = '\0';
    std::string type_str(type);
    i = 0;
    SkipSpace(ptr);
    
    if (segment[ptr] == '(')
    {
        ++ptr;
        // get the default name
        int num = 0;
        for (auto& cell : Cells)
        {
            if (type_str == cell->type)
                ++num;
        }
        sprintf(name, "%s_%d", type, num);
    }
    else
    {
        while (isCharacter(segment[ptr]))
        {
            name[i++] = segment[ptr++];
        }
        name[i] = '\0';
        while (segment[ptr] != '(') ++ptr;
        ++ptr;
    }
    
    Cell* cell = new Cell(name, type, ++cellNum);
    GetNetinCell(ptr, cell->SignleNets, cell->MultiNets);
    Cells.emplace_back(cell);
    PutCellintoNet(cell);
}


void Parser::PutCellintoNet(Cell* cell)
{
    bool flag = 0;
    for (auto& netName : cell->SignleNets)
    {
        auto iter = SingleNets.find(netName);
        if (iter != SingleNets.end())
        {
            iter->second->Cells.insert(cell);
        }
        else
        {
            Net* net = new Net(netName, 0, 1);
            net->Cells.insert(cell);
            SingleNets.insert(std::pair<std::string, Net*>(netName, net));
            fprintf(errorLogFile, "ERROR! DO NOT FIND SINGLE NET %s\n", netName.c_str());
            fprintf(errorLogFile, "ADD SINGLE NET %s !\n", netName.c_str());
        }
            
    }

    for (auto& mulNetStruct : cell->MultiNets)
    {
        auto iter = MultiNets.find(mulNetStruct.name);
        if (iter != MultiNets.end())
        {
            int maxBit = iter->second[0]->ID;
            int offset = maxBit - mulNetStruct.ID;
            iter->second[offset]->Cells.insert(cell);
        }
        else
        {
            std::string netName = mulNetStruct.name + "[" + std::to_string(mulNetStruct.ID) + "]";
            auto iter = SingleNets.find(netName);
            if (iter != SingleNets.end())
            {
                iter->second->Cells.insert(cell);
            }
            else
            {
                fprintf(errorLogFile, "DO NOT FIND MULTI NET %s [%0d]\n", mulNetStruct.name.c_str(), mulNetStruct.ID);
            }
        }
    }
}


void Parser::GetNetinCell(size_t& ptr, std::vector <std::string>& Single_Nets_in_Cell, std::vector <MultiNet>& Multi_Nets_in_Cell)
{
    char netName[SIZE_OF_NAME] = {'\0'};
    memset(netName, '\0', SIZE_OF_NAME * sizeof(char));
    char num[8] = {'\0'};
    while(segment[ptr] != ')')  // the last ')'
    {
        size_t i = 0;
        int bitNum = -1;
        while (segment[ptr] != '(') ++ptr;
        ++ptr; 
        while (segment[ptr] == ' ') ++ptr;
        while (segment[ptr] != ')')
        {
            if (isCharacter(segment[ptr]))
            {
                if (segment[ptr] == '[')
                {
                    ++ptr; SkipSpace(ptr);
                    size_t j = 0;
                    while (segment[ptr] != ']')
                    {
                        num[j++] = segment[ptr++];
                    }
                    ++ptr; num[j] = '\0';
                    if ((isCharacter(segment[ptr])) && (segment[ptr] != ')'))  // xxx[0]~wirecell
                    {
                        netName[i] = '\0';
                        sprintf(netName, "%s[%s]", netName, num);
                        i += (j + 2);
                        j = 0;
                        while (isCharacter(segment[ptr]))
                        {
                            netName[i++] = segment[ptr++];
                        }
                        num[0] = '-'; num[1] = '1'; j = 2;
                        ++ptr; 
                    }
                    SkipSpace(ptr);
                    num[j] = '\0';
                    if (segment[ptr] == '[')  // xxx[0] [1]
                    {
                        ++ptr;
                        netName[i] = '\0';
                        sprintf(netName, "%s[%s]", netName, num);
                        i += (j + 2);
                        j = 0;
                        while (segment[ptr] != ']')
                        {
                            num[j++] = segment[ptr++];
                        }
                        ++ptr; SkipSpace(ptr);
                        num[j] = '\0';
                    }
                    bitNum = str2int(num);
                    memset(num, '\0', 8 * sizeof(char));
                }
                else
                {
                    netName[i++] = segment[ptr++];
                }
            }
            else if (segment[ptr] == '{')  // handle {net0, net1, ...}
            {
                HandleCombinedNets(ptr, Single_Nets_in_Cell, Multi_Nets_in_Cell);
                break;
            }
            else
                ++ptr;
        }
        netName[i] = '\0';
        ++ptr;
        SkipSpace(ptr);
        std::string str(netName);
        memset(netName, '\0', SIZE_OF_NAME * sizeof(char));
        if (str != "")
        {
            if (bitNum < 0)
            {
                Single_Nets_in_Cell.emplace_back(str);
            }
            else
            {
                MultiNet* mn = new MultiNet(str, bitNum);
                Multi_Nets_in_Cell.emplace_back(*mn);
            }
        }
        
    }    
}


void Parser::HandleCombinedNets(size_t& ptr, std::vector <std::string>& Single_Nets_in_Cell, std::vector <MultiNet>& Multi_Nets_in_Cell)
{
    ++ptr;
    SkipSpace(ptr);
    char netName[SIZE_OF_NAME] = {'\0'};
    char num[8] = {'\0'};
    int bitNum = -1;
    size_t i = 0;
    while (segment[ptr] != '}')
    {
        if (isCharacter(segment[ptr]))
        {
            if (segment[ptr] == '[')
            {
                ++ptr; SkipSpace(ptr);
                size_t j = 0;
                while (segment[ptr] != ']')
                {
                    num[j++] = segment[ptr++];
                }
                ++ptr; 
                if (isCharacter(segment[ptr]) && (segment[ptr] != ')'))  // xxx[0]~wirecell
                {
                    netName[i] = '\0';
                    sprintf(netName, "%s[%s]", netName, num);
                    i += (j + 2);
                    j = 0;
                    while (isCharacter(segment[ptr]))
                    {
                        netName[i++] = segment[ptr++];
                    }
                    num[0] = '-'; num[1] = '1'; j = 2;
                    ++ptr;
                }
                SkipSpace(ptr);
                num[j] = '\0';
                if (segment[ptr] == '[')  // xxx[0] [1]
                {
                    ++ptr;
                    netName[i] = '\0';
                    sprintf(netName, "%s[%s]", netName, num);
                    i += (j + 2);
                    j = 0;
                    while (segment[ptr] != ']')
                    {
                        num[j++] = segment[ptr++];
                    }
                    ++ptr; SkipSpace(ptr);
                    num[j] = '\0';
                }
                bitNum = str2int(num);
                memset(num, '\0', 8 * sizeof(char));
            }
            else
            {
                netName[i++] = segment[ptr++];
            }
        }
        else if (segment[ptr] == ',')
        {
            netName[i] = '\0';
            ++ptr;
            SkipSpace(ptr);
            std::string str(netName);
            memset(netName, '\0', SIZE_OF_NAME * sizeof(char));
            if (str != "")
            {
                if (bitNum < 0)
                {
                    Single_Nets_in_Cell.emplace_back(str);
                }
                else
                {
                    MultiNet* mn = new MultiNet(str, bitNum);
                    Multi_Nets_in_Cell.emplace_back(*mn);
                }
            }
            bitNum = -1;
            i = 0;
        }
        else
        {
            ++ptr;
        }
    }
    netName[i] = '\0';
    ++ptr;
    SkipSpace(ptr);
    std::string str(netName);
    memset(netName, '\0', SIZE_OF_NAME * sizeof(char));
    if (str != "")
    {
        if (bitNum < 0)
        {
            Single_Nets_in_Cell.emplace_back(str);
        }
        else
        {
            MultiNet* mn = new MultiNet(str, bitNum);
            Multi_Nets_in_Cell.emplace_back(*mn);
        }
    }
}


bool Parser::isCharacter(char& cc)
{
    if (((cc >= '0') && (cc <= '9')) 
     || ((cc >= 'a') && (cc <= 'z')) 
     || ((cc >= 'A') && (cc <= 'Z')) 
     || ((cc == '_') || (cc == '[') || (cc == ']') || (cc == ':') || (cc == '.'))
     || ((cc == '(') || (cc == ')'))
     || ((cc == '\\') || (cc == '/') || (cc == '|') || (cc == '~')))
    {
        return true;
    }
    else
    {
        return false;
    }
}


bool Parser::isStr(const char* str, size_t& ptr)
{
    SkipSpace(ptr);

    for (size_t i = 0; i < strlen(str); ++i)
    {
        if(segment[ptr++] == str[i])
        {
            continue;
        }
        else
        {
            ptr -= (i+1);
            return false;
        }
    }

    return true;
}


void Parser::SkipSpace(size_t& ptr)
{
    while ((segment[ptr] == ' ') || (segment[ptr] == '\n')
             || (segment[ptr] == '\t') || (segment[ptr] == '\r'))
        ++ptr;
}


void Parser::ShowCells()
{
    printf("Start writing in %s\nwait ...\n", showCellsFileName.c_str());
    FILE* fs = fopen(showCellsFileName.c_str(), "w");

    for (auto cell : Cells)
    {
        fprintf(fs, "Cell %s : %ld relative nets\n", cell->name.c_str(), cell->SignleNets.size() + cell->MultiNets.size());
        for (auto& net : cell->SignleNets)
        {
            fprintf(fs, "\t%s\n", net.c_str());
        }
        for (auto& net : cell->MultiNets)
        {
            fprintf(fs, "\t%s [%0d]\n", net.name.c_str(), net.ID);
        }
    }

    fclose(fs);
    printf("Write in %s Success !\n", showCellsFileName.c_str());
}


void Parser::ShowNets()
{
    printf("Start writing in %s\nwait ...\n", showNetsFileName.c_str());
    FILE* fs = fopen(showNetsFileName.c_str(), "w");

    for (auto& net : SingleNets)
    {
        fprintf(fs, "SIGNLE Net %s : %ld relative cells\n", net.second->name.c_str(), net.second->Cells.size());
        for (auto cell : net.second->Cells)
        {
            fprintf(fs, "\t%s\n", cell->name.c_str());
        }
    }

    for (auto& multi_net : MultiNets)
    {
        for (auto& net : multi_net.second)
        {
            fprintf(fs, "MULTI  Net %s [%0d] : %ld relative cells\n", net->name.c_str(), net->ID, net->Cells.size());
            for (auto cell : net->Cells)
            {
                fprintf(fs, "\t%s\n", cell->name.c_str());
            }
        }
    }

    fclose(fs);
    printf("Write in %s Success !\n", showNetsFileName.c_str());
}


void Parser::WriteCell2CodeDecoder()
{
    printf("Start writing in %s\nwait ...\n", outputCell2CodeCsvName.c_str());
    FILE* fs = fopen(outputCell2CodeCsvName.c_str(), "w");

    fprintf(fs, "cellName, code\n");
    for (auto cell : Cells)
    {
        fprintf(fs, "%s, %0d\n", cell->name.c_str(), cell->ID);
    }

    fclose(fs);
    printf("Write in %s Success !\n", outputCell2CodeCsvName.c_str());
}  


void Parser::WriteHgr()
{
    printf("Start writing in %s\nwait ...\n", outputHgrFileName.c_str());
    FILE* fs = fopen(outputHgrFileName.c_str(), "w");

    GetNetNum();
    fprintf(fs, "%d %d\n", netNum, cellNum);

    for (auto& net : SingleNets)
    {
        if (net.second->Cells.size() > 0)
        {    
            for (auto& cell : net.second->Cells)
            {
                fprintf(fs, "%d ", cell->ID);
            }
            fprintf(fs, "\n");
        }
    }

    for (auto& multi_net : MultiNets)
    {
        for (auto& net : multi_net.second)
        {
            if (net->Cells.size() > 0)
            {
                for (auto& cell : net->Cells)
                {
                    fprintf(fs, "%d ", cell->ID);
                }
                fprintf(fs, "\n");
            }  
        }
           
    }

    fclose(fs);
    printf("Write in %s Success !\n", outputHgrFileName.c_str());
}


void Parser::GetNetNum()
{
    for (auto& net : SingleNets)
    {
        if (net.second->Cells.size() > 0) ++netNum;
    }
    for (auto iter = MultiNets.begin(); iter != MultiNets.end(); ++iter)
    {
        for (auto net : iter->second)
        {
            if (net->Cells.size() > 0) ++netNum;
        }
    }
}


void Parser::Parse(std::string fileName)
{
    ReadFile(fileName);
    std::cout << "File buffer length = " << len_of_buf << std::endl;
    int precent = 0;
    int precent_r = 0;

    while (1)
    {
        if (GetSegment())  // skip module(...);
        {
            while (1)
            {
                GetSegment();

                precent = ptr_of_buf * 100/len_of_buf;
                if (precent != precent_r)
                {
                    printf("\r%3d / 100", precent);
                    precent_r = precent;
                }
                size_t ptr = 0;
                if (!isStr("endmodule", ptr))
                {
                    if (isStr("wire ", ptr) || isStr("input ", ptr) || isStr("output ", ptr) || isStr("inout ", ptr))
                    {
                        GetNets(ptr);
                    }
                    else if ((isStr("defparam ", ptr)) || (isStr("assign ", ptr)))
                    {
                        continue;
                    }
                    else
                    {
                        GetCells(ptr);
                    }
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            break;
        }
    }

    printf("\r 100 / 100\n");
    WriteCell2CodeDecoder();
    WriteHgr();
    printf("Cells Num = %0d\n", cellNum);
    printf("Nets Num = %0ld\n", SingleNets.size() + MultiNets.size());

}