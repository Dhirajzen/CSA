#include<iostream>
#include<string>
#include<vector>
#include<bitset>
#include<fstream>

using namespace std;

#define MemSize 1000 // memory size, in reality, the memory size should be 2^32, but for this lab, for the space resaon, we keep it as this large number, but the memory is still 32-bit addressable.

struct IFStruct {
    bitset<32>  PC;
    bool        nop;  
};

struct IDStruct {
    bitset<32>  Instr;
    bool        nop;  
};

struct EXStruct {
    bitset<32>  Read_data1;
    bitset<32>  Read_data2;
    bitset<16>  Imm;
    bitset<5>   Rs;
    bitset<5>   Rt;
    bitset<5>   Wrt_reg_addr;
    bool        is_I_type;
    bool        rd_mem;
    bool        wrt_mem; 
    bool        alu_op;     //1 for addu, lw, sw, 0 for subu 
    bool        wrt_enable;
    bool        nop;  
};

struct MEMStruct {
    bitset<32>  ALUresult;
    bitset<32>  Store_data;
    bitset<5>   Rs;
    bitset<5>   Rt;    
    bitset<5>   Wrt_reg_addr;
    bool        rd_mem;
    bool        wrt_mem; 
    bool        wrt_enable;    
    bool        nop;    
};

struct WBStruct {
    bitset<32>  Wrt_data;
    bitset<5>   Rs;
    bitset<5>   Rt;     
    bitset<5>   Wrt_reg_addr;
    bool        wrt_enable;
    bool        nop;     
};

struct stateStruct {
    IFStruct    IF;
    IDStruct    ID;
    EXStruct    EX;
    MEMStruct   MEM;
    WBStruct    WB;
};

class InsMem {
public:
    string id, ioDir;
    InsMem(string name, string ioDir) {       
        id = name;
        IMem.resize(MemSize);
        ifstream imem;
        string line;
        int i=0;
        imem.open(ioDir + "/imem.txt");
        if (imem.is_open()) {
            while (getline(imem,line)) {
                try {
                    // Debug output
                    cout << "Reading line: " << line << endl;
                    
                    // Remove any whitespace
                    line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
                    
                    // Validate line contains only 0s and 1s
                    if(line.find_first_not_of("01") != string::npos) {
                        cout << "Invalid characters in line: " << line << endl;
                        continue;
                    }
                    
                    // Pad to 8 bits if necessary
                    while(line.length() < 8) {
                        line = "0" + line;
                    }
                    // Truncate to 8 bits if too long
                    if(line.length() > 8) {
                        line = line.substr(0, 8);
                    }
                    
                    IMem[i] = bitset<8>(line);
                    i++;
                }
                catch(const exception& e) {
                    cout << "Error processing line: " << line << endl;
                    cout << "Error message: " << e.what() << endl;
                }
                
            }
        }
        else cout << "Unable to open IMEM input file.";
        imem.close();
    }

    bitset<32> readInstr(bitset<32> ReadAddress) {
        try {
            string instr = "";
            int addr = ReadAddress.to_ulong();
            
            // Bounds check
            if(addr + 3 >= MemSize) {
                cout << "Address out of bounds: " << addr << endl;
                return bitset<32>(0);
            }
            
            for(int i = 0; i < 4; i++) {
                string byte = IMem[addr+i].to_string();
                // Ensure each byte is 8 bits
                while(byte.length() < 8) {
                    byte = "0" + byte;
                }
                instr += byte;
            }
            
            // Debug output
            cout << "Reading instruction at address " << addr << ": " << instr << endl;
            
            return bitset<32>(instr);
        }
        catch(const exception& e) {
            cout << "Error reading instruction: " << e.what() << endl;
            return bitset<32>(0);
        }
    }

private:
    vector<bitset<8> > IMem;
};

class DataMem {
public:
    string id, opFilePath, ioDir;
    DataMem(string name, string ioDir) : id(name), ioDir(ioDir) {
        DMem.resize(MemSize);
        opFilePath = ioDir + "/" + name + "_DMEMResult.txt";
        ifstream dmem;
        string line;
        int i=0;
        dmem.open(ioDir + "/dmem.txt");
        if (dmem.is_open()) {
            while (getline(dmem,line)) {
                try {
                    // Debug output
                    cout << "Reading line: " << line << endl;
                    
                    // Remove any whitespace
                    line.erase(remove_if(line.begin(), line.end(), ::isspace), line.end());
                    
                    // Validate line contains only 0s and 1s
                    if(line.find_first_not_of("01") != string::npos) {
                        cout << "Invalid characters in line: " << line << endl;
                        continue;
                    }
                    
                    // Pad to 8 bits if necessary
                    while(line.length() < 8) {
                        line = "0" + line;
                    }
                    // Truncate to 8 bits if too long
                    if(line.length() > 8) {
                        line = line.substr(0, 8);
                    }
                    
                    DMem[i] = bitset<8>(line);
                    i++;
                }
                catch(const exception& e) {
                    cout << "Error processing line: " << line << endl;
                    cout << "Error message: " << e.what() << endl;
                }
            }
        }
        else cout << "Unable to open DMEM input file.";
        dmem.close();
    }

    bitset<32> readDataMem(bitset<32> Address) {
        string data = "";
        int mem = Address.to_ulong();
        
        for(int i = 0; i < 4; i++) {
            string byte = DMem[mem+i].to_string();
            while(byte.length() < 8) {
                byte = "0" + byte;
            }
            data += byte;
        }
            
        return bitset<32>(data);
    }
            
        void writeDataMem(bitset<32> Address, bitset<32> WriteData) {
			// write into memory
            int addr = Address.to_ulong();
            string data = WriteData.to_string();
            // Split 32-bit data into 4 bytes
            DMem[addr]   = bitset<8>(data.substr(0, 8));    // First byte
            DMem[addr+1] = bitset<8>(data.substr(8, 8));    // Second byte
            DMem[addr+2] = bitset<8>(data.substr(16, 8));   // Third byte
            DMem[addr+3] = bitset<8>(data.substr(24, 8));   // Fourth byte

        }   

        void printMemory() {
        cout << "Current memory contents:" << endl;
        for(int i = 0; i < 12; i++) {  // Print first 12 bytes
            cout << "DMem[" << i << "]: " << DMem[i] << endl;
        }
        }
                     
        void outputDataMem() {
            ofstream dmemout;
            dmemout.open(opFilePath, std::ios_base::trunc);
            
            if (dmemout.is_open()) {
                for (int j = 0; j< 1000; j++)
                {     
                    dmemout << DMem[j] << endl;   
                }
                     
            }
            else cout<<"Unable to open "<<id<<" DMEM result file." << endl;
            
            dmemout.close();
        }             

    private:
		vector<bitset<8> > DMem;      
};

class RegisterFile
{
    public:
		string outputFile;
     	RegisterFile(string ioDir): outputFile (ioDir + "RFResult.txt") {
			Registers.resize(32);   //creating 32 32-bit Registers
			Registers[0] = bitset<32> (0); //register R0 hardwired to 0
        }
	
        bitset<32> readRF(bitset<5> Reg_addr) {   
            return Registers[Reg_addr.to_ulong()];  //return the value stored in the given register address
        }
    
        void writeRF(bitset<5> Reg_addr, bitset<32> Wrt_reg_data) {
            int regNum = Reg_addr.to_ulong();       //get the address as an index 
            
            if(regNum != 0){
                Registers[regNum] = Wrt_reg_data;   //check is it's not R0 and then write into the register
            } 
        }
		 
		void outputRF(int cycle) {
			ofstream rfout;
			if (cycle == 0)
				rfout.open(outputFile, std::ios_base::trunc);
			else 
				rfout.open(outputFile, std::ios_base::app);
			if (rfout.is_open())
			{
				rfout<<"State of RF after executing cycle:\t"<<cycle<<endl;
				for (int j = 0; j<32; j++)
				{
					rfout << Registers[j]<<endl;
				}
			}
			else cout<<"Unable to open RF output file."<<endl;
			rfout.close();               
		} 
			
	private:
		vector<bitset<32> >Registers;
};

class Core {
	public:
		RegisterFile myRF;
		uint32_t cycle;
		bool halted;
		string ioDir;
		struct stateStruct state, nextState;
		InsMem &ext_imem;
		DataMem &ext_dmem;
		
		Core(string ioDir, InsMem &imem, DataMem &dmem): 
            myRF(ioDir), 
            ioDir(ioDir), 
            ext_imem (imem), 
            ext_dmem (dmem), 
            cycle(0), 
            halted(false) {}

		virtual void step() {}

		virtual void printState() {}
};

class SingleStageCore : public Core {
	public:
		SingleStageCore(string ioDir, InsMem &imem, DataMem &dmem): Core(ioDir + "/SS_", imem, dmem), opFilePath(ioDir + "/StateResult_SS.txt") {}

		void step() {
            if(state.IF.PC.to_ulong()<1000){
                //IF stage
                bitset<32> instruction = ext_imem.readInstr(state.IF.PC);

                //ID stage
                //first check the opcode for instruction type
                bitset<7> opcode = bitset<7>(instruction.to_string().substr(25, 7));

                bitset<5> rs1, rs2, rd;
                bitset<3> func3;
                bitset<7> func7;
                bitset<12> imm;
                bitset<32> rs1_value, rs2_value, rs1_data, rs2_data, addr, data, result;

                // Debug print
                cout << "PC: " << state.IF.PC.to_ulong() 
                    << " Instruction: " << instruction 
                    << " Opcode: " << opcode << endl;
                
                switch (opcode.to_ulong())
                {
                case 0x33:  //R-type
                    //get value of other fields of this instruction
                    rs1 = bitset<5>(instruction.to_string().substr(12, 5));
                    rs2 = bitset<5>(instruction.to_string().substr(7, 5));
                    rd = bitset<5>(instruction.to_string().substr(20, 5));

                    func7 = bitset<7>(instruction.to_string().substr(0, 7));
                    func3 = bitset<3>(instruction.to_string().substr(17, 3));

                    rs1_value = myRF.readRF(rs1);
                    rs2_value = myRF.readRF(rs2);

                    if(func7.to_ulong() == 0x0){
                        switch (func3.to_ulong())
                        {
                        case 0x0:   //ADD
                            result=bitset<32>(rs1_value.to_ulong()+rs2_value.to_ulong());
                            break;
                        
                        case 0x4:   //XOR
                            result= rs1_value ^ rs2_value;
                            break;
                        
                        case 0x6:   //OR
                            result= rs1_value | rs2_value;
                            break;

                        case 0x7:   //AND
                            result= rs1_value & rs2_value;
                            break;
                        
                        default:
                            break;
                        }
                    }
                    else{
                        //SUB
                        result=bitset<32>(rs1_value.to_ulong()-rs2_value.to_ulong());
                    }

                    myRF.writeRF(rd,result);
                    break;

                case 0x03: case 0x13:  //I-type
                    //get value of other fields of this instruction
                    imm = bitset<12>(instruction.to_string().substr(0, 12));
                    rs1 = bitset<5>(instruction.to_string().substr(12, 5));
                    rd = bitset<5>(instruction.to_string().substr(20, 5));
                    func3 = bitset<3>(instruction.to_string().substr(17, 3));

                    rs1_data = myRF.readRF(rs1);

                    if(opcode.to_ulong()==0x03 and func3.to_ulong()==0x00){
                        //LW
                        addr = bitset<32>(rs1_data.to_ulong()+imm.to_ulong());
                        data = ext_dmem.readDataMem(addr);
                        myRF.writeRF(rd,data);

                    }

                    else{
                        switch (func3.to_ulong())
                        {
                        case 0x0:
                            //ADDI
                            break;
                        
                        case 0x4:
                            //XORI
                            break;

                        case 0x6:
                            //ORI
                            break;
                        
                        case 0x7:
                            //ANDI
                            break;
                        
                        default:
                            break;
                        }
                    }
                    break;
                case 0x23:  //S-type
                    //get value of other fields of this instruction
                    rs1 = bitset<5>(instruction.to_string().substr(12, 5));
                    rs2 = bitset<5>(instruction.to_string().substr(7, 5));
                    func3 = bitset<3>(instruction.to_string().substr(17, 3));
                    imm = bitset<12>(instruction.to_string().substr(0, 7) + instruction.to_string().substr(20, 5));

                    // cout << "rs1: " << rs1 << " rs2: " << rs2 << " imm: " << imm << endl;

                    rs1_data=myRF.readRF(rs1);
                    rs2_data=myRF.readRF(rs2);
                    addr = (rs1_data.to_ulong()+imm.to_ulong());

                    // cout << "rs1_data: " << rs1_data << endl;
                    // cout << "rs2_data: " << rs2_data << endl;
                    // cout << "Store address: " << addr << endl;
                    
                    //SW
                    ext_dmem.writeDataMem(addr,rs2_data);

                    break;
                case 0x7F: //HALT
                    // Set nop flag in next state
                    nextState.IF.nop = true;
                    // Keep PC at current value (don't increment)
                    nextState.IF.PC = state.IF.PC;

                    break;
                default:
                    break;
                }

                if(!nextState.IF.nop) {
                    nextState.IF.PC = state.IF.PC.to_ulong() + 4;
                }
        
                if (nextState.IF.nop){
                    halted = true;
                }
                    
                myRF.outputRF(cycle); // dump RF
                printState(nextState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 
                
                state = nextState; // The end of the cycle and updates the current state with the values calculated in this cycle
                cycle++;
            }
            else{
                halted = true;
            }
			
		}

		void printState(stateStruct state, int cycle) {
    		ofstream printstate;
			if (cycle == 0)
				printstate.open(opFilePath, std::ios_base::trunc);
			else 
    			printstate.open(opFilePath, std::ios_base::app);
    		if (printstate.is_open()) {
    		    printstate<<"State after executing cycle:\t"<<cycle<<endl; 

    		    printstate<<"IF.PC:\t"<<state.IF.PC.to_ulong()<<endl;
    		    printstate<<"IF.nop:\t"<<state.IF.nop<<endl;
    		}
    		else cout<<"Unable to open SS StateResult output file." << endl;
    		printstate.close();
		}
	private:
		string opFilePath;
};

class FiveStageCore : public Core{
	public:
		
		FiveStageCore(string ioDir, InsMem &imem, DataMem &dmem): Core(ioDir + "\\FS_", imem, dmem), opFilePath(ioDir + "\\StateResult_FS.txt") {}

		void step() {
			/* Your implementation */
			/* --------------------- WB stage --------------------- */
			
			
			
			/* --------------------- MEM stage -------------------- */
			
			
			
			/* --------------------- EX stage --------------------- */
			
			
			
			/* --------------------- ID stage --------------------- */
			
			
			
			/* --------------------- IF stage --------------------- */
			
			
			halted = true;
			if (state.IF.nop && state.ID.nop && state.EX.nop && state.MEM.nop && state.WB.nop)
				halted = true;
        
            myRF.outputRF(cycle); // dump RF
			printState(nextState, cycle); //print states after executing cycle 0, cycle 1, cycle 2 ... 
       
			state = nextState; //The end of the cycle and updates the current state with the values calculated in this cycle
			cycle++;
		}

		void printState(stateStruct state, int cycle) {
		    ofstream printstate;
			if (cycle == 0)
				printstate.open(opFilePath, std::ios_base::trunc);
			else 
		    	printstate.open(opFilePath, std::ios_base::app);
		    if (printstate.is_open()) {
		        printstate<<"State after executing cycle:\t"<<cycle<<endl; 

		        printstate<<"IF.PC:\t"<<state.IF.PC.to_ulong()<<endl;        
		        printstate<<"IF.nop:\t"<<state.IF.nop<<endl; 

		        printstate<<"ID.Instr:\t"<<state.ID.Instr<<endl; 
		        printstate<<"ID.nop:\t"<<state.ID.nop<<endl;

		        printstate<<"EX.Read_data1:\t"<<state.EX.Read_data1<<endl;
		        printstate<<"EX.Read_data2:\t"<<state.EX.Read_data2<<endl;
		        printstate<<"EX.Imm:\t"<<state.EX.Imm<<endl; 
		        printstate<<"EX.Rs:\t"<<state.EX.Rs<<endl;
		        printstate<<"EX.Rt:\t"<<state.EX.Rt<<endl;
		        printstate<<"EX.Wrt_reg_addr:\t"<<state.EX.Wrt_reg_addr<<endl;
		        printstate<<"EX.is_I_type:\t"<<state.EX.is_I_type<<endl; 
		        printstate<<"EX.rd_mem:\t"<<state.EX.rd_mem<<endl;
		        printstate<<"EX.wrt_mem:\t"<<state.EX.wrt_mem<<endl;        
		        printstate<<"EX.alu_op:\t"<<state.EX.alu_op<<endl;
		        printstate<<"EX.wrt_enable:\t"<<state.EX.wrt_enable<<endl;
		        printstate<<"EX.nop:\t"<<state.EX.nop<<endl;        

		        printstate<<"MEM.ALUresult:\t"<<state.MEM.ALUresult<<endl;
		        printstate<<"MEM.Store_data:\t"<<state.MEM.Store_data<<endl; 
		        printstate<<"MEM.Rs:\t"<<state.MEM.Rs<<endl;
		        printstate<<"MEM.Rt:\t"<<state.MEM.Rt<<endl;   
		        printstate<<"MEM.Wrt_reg_addr:\t"<<state.MEM.Wrt_reg_addr<<endl;              
		        printstate<<"MEM.rd_mem:\t"<<state.MEM.rd_mem<<endl;
		        printstate<<"MEM.wrt_mem:\t"<<state.MEM.wrt_mem<<endl; 
		        printstate<<"MEM.wrt_enable:\t"<<state.MEM.wrt_enable<<endl;         
		        printstate<<"MEM.nop:\t"<<state.MEM.nop<<endl;        

		        printstate<<"WB.Wrt_data:\t"<<state.WB.Wrt_data<<endl;
		        printstate<<"WB.Rs:\t"<<state.WB.Rs<<endl;
		        printstate<<"WB.Rt:\t"<<state.WB.Rt<<endl;
		        printstate<<"WB.Wrt_reg_addr:\t"<<state.WB.Wrt_reg_addr<<endl;
		        printstate<<"WB.wrt_enable:\t"<<state.WB.wrt_enable<<endl;
		        printstate<<"WB.nop:\t"<<state.WB.nop<<endl; 
		    }
		    else cout<<"Unable to open FS StateResult output file." << endl;
		    printstate.close();
		}
	private:
		string opFilePath;
};

int main(int argc, char* argv[]) {
	
	string ioDir = "";
    if (argc == 1) {
        cout << "Enter path containing the memory files: ";
        cin >> ioDir;
    }
    else if (argc > 2) {
        cout << "Invalid number of arguments. Machine stopped." << endl;
        return -1;
    }
    else {
        ioDir = argv[1];
        cout << "IO Directory: " << ioDir << endl;
    }

    InsMem imem = InsMem("Imem", ioDir);
    DataMem dmem_ss = DataMem("SS", ioDir);
	// DataMem dmem_fs = DataMem("FS", ioDir);

	SingleStageCore SSCore(ioDir, imem, dmem_ss);
	// FiveStageCore FSCore(ioDir, imem, dmem_fs);

    while (1) {
		if (!SSCore.halted)
			SSCore.step();
		
		// if (!FSCore.halted)
		// 	FSCore.step();

		// if (SSCore.halted && FSCore.halted)
		// 	break;

        if (SSCore.halted)
			break;
    }

dmem_ss.outputDataMem();

	return 0;

}