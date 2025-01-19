#include<iostream>
#include<string>
#include<vector>
#include<bitset>
#include<fstream>
#include<iomanip>

using namespace std;

#define MemSize 1000 // memory size, in reality, the memory size should be 2^32, but for this lab, for the space resaon, we keep it as this large number, but the memory is still 32-bit addressable.

struct IFStruct {
    bitset<32>  PC;
    bool        nop;  
};

struct IDStruct {
    bitset<32>  Instr;
    bool        nop;
    bool        is_hazard;
    bool        is_I_type;  
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
    string       alu_op;     //1 for addu, lw, sw, 0 for subu 
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

class InsMem
{
	public:
		string id, ioDir;
        InsMem(string name, string ioDir) {       
			id = name;
			IMem.resize(MemSize);
            ifstream imem;
			string line;
			int i=0;
			imem.open(ioDir + "/imem.txt");
			if (imem.is_open())
			{
				while (getline(imem,line))
				{      
					IMem[i] = bitset<8>(line);
					i++;
				}                    
			}else
            {
             
            cout<<"Unable to open IMEM input file.";
            }
			imem.close();                     
		}

		bitset<32> readInstr(bitset<32> ReadAddress) {       // reads instruction as 4 bytes from imem.txt files
			string instr ="";
            int addr=ReadAddress.to_ulong();

            for(int i=0;i<4;i++) 
            {
                     instr += IMem[addr+i].to_string();    
            }

            return bitset<32>(instr);

			
		}     
      
    private:
        vector<bitset<8> > IMem;     
};
      
class DataMem    
{
    public: 

		string id, opFilePath, ioDir;
        DataMem(string name, string ioDir) : id{name}, ioDir{ioDir} {
            DMem.resize(MemSize);
			opFilePath = ioDir + "/" + name + "_DMEMResult.txt";
            ifstream dmem;
            string line;
            int i=0;
            dmem.open(ioDir + "/dmem.txt");
            if (dmem.is_open())
            {
                while (getline(dmem,line))
                {      
                    DMem[i] = bitset<8>(line);
                    i++;
                }
            }
            else cout<<"Unable to open DMEM input file.";
                dmem.close();    

                     
        }
		
        bitset<32> readDataMem(bitset<32> Address) {	  // reads data from dmem.txt as 4 bytes, this gets called when we encounter load instruction.
			string data ="";
            int mem=Address.to_ulong();

            for(int i=0;i<4;i++)
            {
                     data += DMem[mem+i].to_string();
            }

            return bitset<32>(data);


		}
            
        void writeDataMem(bitset<32> Address, bitset<32> WriteData) {    // writes 4 bytes of data to DMem[] which gets stored in the addresses of dmemresult, this gets called when we encounter store instruction.
            
			string data=WriteData.to_string();
            DMem[Address.to_ulong()]=bitset<8>(data.substr(0,8));
             DMem[Address.to_ulong()+1]=bitset<8>(data.substr(8,8));
              DMem[Address.to_ulong()+2]=bitset<8>(data.substr(16,8));
               DMem[Address.to_ulong()+3]=bitset<8>(data.substr(24,8));

               
        }   
                     
        void outputDataMem() {

          
           
            ofstream dmemout;
            dmemout.open(opFilePath.c_str(), ios::out | ios::trunc);

            if (dmemout.is_open()) {
                for (int j = 0; j< 1000; j++)
                {     
                    dmemout << DMem[j]<<endl;
                    

                    
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
     	RegisterFile(string ioDir): outputFile {ioDir + "RFResult.txt"} {
			Registers.resize(32);  
			Registers[0] = bitset<32> (0);  
        }
	
        bitset<32> readRF(bitset<5> Reg_addr) {    // reads values from register file and returns a 32 bit value stored in that register.

            return Registers[Reg_addr.to_ulong()];    
    
        }
    
        void writeRF(bitset<5> Reg_addr, bitset<32> Wrt_reg_data) { // writes data to register file 
            if(Reg_addr.to_ulong()!=0)
            {
                Registers[Reg_addr.to_ulong()]=Wrt_reg_data;
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
		
		Core(string ioDir, InsMem &imem, DataMem &dmem): myRF(ioDir), ioDir{ioDir}, ext_imem {imem}, ext_dmem {dmem},cycle(0),halted(false) {}
        

		virtual void step() {}

		virtual void printState() {}
};


class FiveStageCore : public Core{
	public:
		
		FiveStageCore(string ioDir, InsMem &imem, DataMem &dmem): Core(ioDir + "\\FS_", imem, dmem), opFilePath(ioDir + "\\StateResult_FS.txt") {
            state.IF.nop=false;
            state.IF.PC=bitset<32>(0);
            // Initialize ID stage
    state.ID.nop = false;  // Changed from true to false
    state.ID.Instr = bitset<32>(0);
    state.ID.is_hazard = false;

           // Initialize EX stage
    state.EX.nop = true;
    state.EX.Read_data1 = bitset<32>(0);
    state.EX.Read_data2 = bitset<32>(0);
    state.EX.Imm = bitset<16>(0);
    state.EX.Rs = bitset<5>(0);
    state.EX.Rt = bitset<5>(0);
    state.EX.Wrt_reg_addr = bitset<5>(0);
    state.EX.is_I_type = false;
    state.EX.rd_mem = false;
    state.EX.wrt_mem = false;
    state.EX.alu_op = "00";  // Changed from empty string to "00"
    state.EX.wrt_enable = false;
    // Initialize MEM stage
    state.MEM.nop = true;
    state.MEM.ALUresult = bitset<32>(0);
    state.MEM.Store_data = bitset<32>(0);
    state.MEM.Rs = bitset<5>(0);
    state.MEM.Rt = bitset<5>(0);
    state.MEM.Wrt_reg_addr = bitset<5>(0);
    state.MEM.rd_mem = false;
    state.MEM.wrt_mem = false;
    state.MEM.wrt_enable = false;
    //initializing WB stage
    state.WB.nop = true;
    state.WB.Wrt_data = bitset<32>(0);
    state.WB.Rs = bitset<5>(0);
    state.WB.Rt = bitset<5>(0);
    state.WB.Wrt_reg_addr = bitset<5>(0);
    state.WB.wrt_enable = false;

        }

		void step() {
            nextState=state;
            
			/* Your implementation */
			/* --------------------- WB stage --------------------- */
            
              if (!state.WB.nop && state.WB.wrt_enable) {
                myRF.writeRF(state.WB.Wrt_reg_addr, state.WB.Wrt_data);
            }
        
			
			
			
			/* --------------------- MEM stage -------------------- */
                        if (!nextState.MEM.nop) {
                // Update WB control signals first
                nextState.WB.Rs = nextState.MEM.Rs;
                nextState.WB.Rt = nextState.MEM.Rt;
                nextState.WB.Wrt_reg_addr = nextState.MEM.Wrt_reg_addr;
                nextState.WB.wrt_enable = nextState.MEM.wrt_enable;

                // Handle memory operations
                if (nextState.MEM.rd_mem) {
                    nextState.WB.Wrt_data = ext_dmem.readDataMem(nextState.MEM.ALUresult);
                }
                else if (nextState.MEM.wrt_mem) {
                    ext_dmem.writeDataMem(nextState.MEM.ALUresult, nextState.MEM.Store_data);
                    
                }
                else {
                    nextState.WB.Wrt_data = nextState.MEM.ALUresult;
                }
            }
       

	
			
			/* --------------------- EX stage --------------------- */
            	if (!nextState.EX.nop) {
                // Set control signals for MEM stage
                nextState.MEM.wrt_enable = nextState.EX.wrt_enable;
                nextState.MEM.Wrt_reg_addr = nextState.EX.Wrt_reg_addr;
                nextState.MEM.rd_mem = nextState.EX.rd_mem;
                nextState.MEM.wrt_mem = nextState.EX.wrt_mem;
                nextState.MEM.Rs = nextState.EX.Rs;
                nextState.MEM.Rt = nextState.EX.Rt;
                nextState.MEM.Store_data = nextState.EX.Read_data2;

                // Execute ALU operations
                if (nextState.EX.alu_op == "add" || nextState.EX.alu_op == "lw" || nextState.EX.alu_op == "sw") {
                    if (nextState.EX.is_I_type) {
                        int32_t signExtendedImm = signExtend(nextState.EX.Imm.to_string(), 12);
                        nextState.MEM.ALUresult = bitset<32>(nextState.EX.Read_data1.to_ulong() + signExtendedImm);
                    } else {
                        nextState.MEM.ALUresult = bitset<32>(nextState.EX.Read_data1.to_ulong() + 
                                                            nextState.EX.Read_data2.to_ulong());
                    }
                }
                else if (nextState.EX.alu_op == "sub") {
                    nextState.MEM.ALUresult = bitset<32>(nextState.EX.Read_data1.to_ulong() - 
                                                        nextState.EX.Read_data2.to_ulong());
                }
                else if (nextState.EX.alu_op == "xor" || nextState.EX.alu_op == "xori") {
                    if (nextState.EX.is_I_type) {
                        nextState.MEM.ALUresult = nextState.EX.Read_data1 ^ 
                                                bitset<32>(nextState.EX.Imm.to_ulong());
                    } else {
                        nextState.MEM.ALUresult = nextState.EX.Read_data1 ^ nextState.EX.Read_data2;
                    }
                }
                else if (nextState.EX.alu_op == "or" || nextState.EX.alu_op == "ori") {
                    if (nextState.EX.is_I_type) {
                        nextState.MEM.ALUresult = nextState.EX.Read_data1 | 
                                                bitset<32>(nextState.EX.Imm.to_ulong());
                    } else {
                        nextState.MEM.ALUresult = nextState.EX.Read_data1 | nextState.EX.Read_data2;
                    }
                }
                else if (nextState.EX.alu_op == "and" || nextState.EX.alu_op == "andi") {
                    if (nextState.EX.is_I_type) {
                        nextState.MEM.ALUresult = nextState.EX.Read_data1 & 
                                                bitset<32>(nextState.EX.Imm.to_ulong());
                    } else {
                        nextState.MEM.ALUresult = nextState.EX.Read_data1 & nextState.EX.Read_data2;
                    }
                }
                else if (nextState.EX.alu_op == "addi") {
                    int32_t signExtendedImm = signExtend(nextState.EX.Imm.to_string(), 12);
                    nextState.MEM.ALUresult = bitset<32>(nextState.EX.Read_data1.to_ulong() + signExtendedImm);
                }
                else if (nextState.EX.alu_op == "jal") {
                    nextState.MEM.ALUresult = nextState.EX.Read_data1;
                }
            }
			/* --------------------- ID stage --------------------- */
         if (!nextState.ID.nop) {
                bitset<32> instruction = nextState.ID.Instr;
                string instr_str = instruction.to_string();
                
                bitset<7> opcode(instr_str.substr(25,7));
                bitset<5> rd(instr_str.substr(20,5));
                bitset<3> funct3(instr_str.substr(17,3));
                bitset<5> rs1(instr_str.substr(12,5));
                bitset<5> rs2(instr_str.substr(7,5));
                bitset<7> funct7(instr_str.substr(0,7));

                // Branch instructions - Handle first for early branch resolution
                if (opcode == bitset<7>("1100011")) {
                    // First check if there are pending writes
                    bool pendingWrite = (nextState.MEM.wrt_enable || nextState.WB.wrt_enable);
                    
                    string imm = instr_str.substr(0,1) + instr_str.substr(24,1) + 
                                instr_str.substr(1,6) + instr_str.substr(20,4) + "0";
                    int32_t offset = signExtend(imm, 13);
                    
                    // Check hazards
                    int hazard_rs1 = detectHazard(rs1);
                    int hazard_rs2 = detectHazard(rs2);
                    
                    if (hazard_rs1 == 3 || hazard_rs2 == 3) {
                        nextState.EX.nop = true;
                        nextState.ID.nop = true;
                        return;
                    }
                    
                    // Get register values with forwarding
                    bitset<32> rs1_val, rs2_val;
                    if (hazard_rs1 == 1) rs1_val = nextState.MEM.ALUresult;
                    else if (hazard_rs1 == 2) rs1_val = nextState.WB.Wrt_data;
                    else rs1_val = myRF.readRF(rs1);
                    
                    if (hazard_rs2 == 1) rs2_val = nextState.MEM.ALUresult;
                    else if (hazard_rs2 == 2) rs2_val = nextState.WB.Wrt_data;
                    else rs2_val = myRF.readRF(rs2);
                    
                    bool take_branch = false;
                    if (funct3 == bitset<3>("000")) take_branch = (rs1_val == rs2_val);        // BEQ
                    else if (funct3 == bitset<3>("001")) take_branch = (rs1_val != rs2_val);   // BNE
                    
                    if (take_branch) {
                        if (!pendingWrite) {
                            // If no pending writes, branch immediately
                            nextState.IF.PC = bitset<32>(nextState.IF.PC.to_ulong() + offset);
                            nextState.ID.nop = true;
                            nextState.IF.nop = false;
                            return;
                        } else {
                            // If there are pending writes, set up branch but let writes complete
                            nextState.IF.PC = bitset<32>(nextState.IF.PC.to_ulong() + offset);
                            nextState.ID.nop = true;
                        }
                    }
                }

                // Reset EX stage registers
                nextState.EX.Read_data1 = bitset<32>(0);
                nextState.EX.Read_data2 = bitset<32>(0);
                nextState.EX.Imm = bitset<16>(0);
                nextState.EX.Rs = bitset<5>(0);
                nextState.EX.Rt = bitset<5>(0);
                nextState.EX.Wrt_reg_addr = bitset<5>(0);
                nextState.EX.is_I_type = false;
                nextState.EX.rd_mem = false;
                nextState.EX.wrt_mem = false;
                nextState.EX.alu_op = "";
                nextState.EX.wrt_enable = false;

                // R-type instructions
                if (opcode == bitset<7>("0110011")) {
                    nextState.EX.Rs = rs1;
                    nextState.EX.Rt = rs2;
                    nextState.EX.Wrt_reg_addr = rd;
                    nextState.EX.wrt_enable = true;
                    
                    int hazard_rs1 = detectHazard(rs1);
                    int hazard_rs2 = detectHazard(rs2);

                    if (hazard_rs1 == 3 || hazard_rs2 == 3) {
                        nextState.EX.nop = true;
                        return;
                    }

                    // Handle forwarding
                    if (hazard_rs1 == 1) nextState.EX.Read_data1 = nextState.MEM.ALUresult;
                    else if (hazard_rs1 == 2) nextState.EX.Read_data1 = nextState.WB.Wrt_data;
                    else nextState.EX.Read_data1 = myRF.readRF(rs1);

                    if (hazard_rs2 == 1) nextState.EX.Read_data2 = nextState.MEM.ALUresult;
                    else if (hazard_rs2 == 2) nextState.EX.Read_data2 = nextState.WB.Wrt_data;
                    else nextState.EX.Read_data2 = myRF.readRF(rs2);

                    // Set ALU operation
                    if (funct3.to_string() == "000") {
                        if (funct7.to_string() == "0000000") nextState.EX.alu_op = "add";
                        else nextState.EX.alu_op = "sub";
                    }
                    else if (funct3.to_string() == "100") nextState.EX.alu_op = "xor";
                    else if (funct3.to_string() == "110") nextState.EX.alu_op = "or";
                    else if (funct3.to_string() == "111") nextState.EX.alu_op = "and";
                }

                // I-type instructions
                else if (opcode == bitset<7>("0010011")) {
                    string imm = instr_str.substr(0, 12);
                    int32_t signExtendedImm = signExtend(imm, 12);
                    
                    nextState.EX.Rs = rs1;
                    nextState.EX.Wrt_reg_addr = rd;
                    nextState.EX.Imm = bitset<16>(signExtendedImm & 0xFFFF);
                    nextState.EX.wrt_enable = true;
                    nextState.EX.is_I_type = true;

                    int hazard_rs1 = detectHazard(rs1);
                    if (hazard_rs1 == 3) {
                        nextState.EX.nop = true;
                        return;
                    }

                    // Handle forwarding
                    if (hazard_rs1 == 1) nextState.EX.Read_data1 = nextState.MEM.ALUresult;
                    else if (hazard_rs1 == 2) nextState.EX.Read_data1 = nextState.WB.Wrt_data;
                    else nextState.EX.Read_data1 = myRF.readRF(rs1);

                    // Set ALU operation
                    if (funct3 == bitset<3>("000")) nextState.EX.alu_op = "addi";
                    else if (funct3 == bitset<3>("100")) nextState.EX.alu_op = "xori";
                    else if (funct3 == bitset<3>("110")) nextState.EX.alu_op = "ori";
                    else if (funct3 == bitset<3>("111")) nextState.EX.alu_op = "andi";
                }
                else if (opcode == bitset<7>("0000011")) {
                    string imm = instr_str.substr(0, 12);
                    int32_t signExtendedImm = signExtend(imm, 12);
                    
                    nextState.EX.Rs = rs1;
                    nextState.EX.Wrt_reg_addr = rd;
                    nextState.EX.Imm = bitset<16>(signExtendedImm & 0xFFFF);
                    nextState.EX.rd_mem = true;
                    nextState.EX.wrt_enable = true;
                    nextState.EX.is_I_type = true;
                    nextState.EX.alu_op = "00";

                    int hazard_rs1 = detectHazard(rs1);
                    if (hazard_rs1 == 3) {
                        nextState.EX.nop = true;
                        nextState.ID.is_hazard = true;
                        return;
                    }

                    // Handle forwarding
                    if (hazard_rs1 == 1) nextState.EX.Read_data1 = nextState.MEM.ALUresult;
                    else if (hazard_rs1 == 2) nextState.EX.Read_data1 = nextState.WB.Wrt_data;
                    else nextState.EX.Read_data1 = myRF.readRF(rs1);
                }

                // Store instructions
                else if (opcode == bitset<7>("0100011")) {
                    string imm = instr_str.substr(0, 7) + instr_str.substr(20, 5);
                    int32_t signExtendedImm = signExtend(imm, 12);
                    
                    nextState.EX.Rs = rs1;
                    nextState.EX.Rt = rs2;
                    nextState.EX.Imm = bitset<16>(signExtendedImm & 0xFFFF);
                    nextState.EX.is_I_type = true;
                    nextState.EX.wrt_mem = true;
                    nextState.EX.alu_op = "sw";

                    int hazard_rs1 = detectHazard(rs1);
                    int hazard_rs2 = detectHazard(rs2);

                    if (hazard_rs1 == 3 || hazard_rs2 == 3) {
                        nextState.EX.nop = true;
                        nextState.ID.is_hazard = true;
                        return;
                    }

                    // Handle forwarding
                    if (hazard_rs1 == 1) nextState.EX.Read_data1 = nextState.MEM.ALUresult;
                    else if (hazard_rs1 == 2) nextState.EX.Read_data1 = nextState.WB.Wrt_data;
                    else nextState.EX.Read_data1 = myRF.readRF(rs1);

                    if (hazard_rs2 == 1) nextState.EX.Read_data2 = nextState.MEM.ALUresult;
                    else if (hazard_rs2 == 2) nextState.EX.Read_data2 = nextState.WB.Wrt_data;
                    else nextState.EX.Read_data2 = myRF.readRF(rs2);
                }

                // JAL instruction
                else if (opcode == bitset<7>("1101111")) {
                    string imm20 = instr_str.substr(0, 1);
                    string imm19_12 = instr_str.substr(12, 8);
                    string imm11 = instr_str.substr(11, 1);
                    string imm10_1 = instr_str.substr(1, 10);
                    string imm = imm20 + imm19_12 + imm11 + imm10_1 + "0";
                    
                    int32_t signExtendedImm = signExtend(imm, 21);
                    bool pendingWrite = (nextState.MEM.wrt_enable || nextState.WB.wrt_enable);
                    
                    nextState.EX.Wrt_reg_addr = rd;
                    nextState.EX.Read_data1 = bitset<32>(nextState.IF.PC.to_ulong() + 4);
                    nextState.EX.wrt_enable = true;
                    nextState.EX.alu_op = "jal";
                    nextState.EX.is_I_type = true;

                    if (!pendingWrite) {
                        nextState.IF.PC = bitset<32>(nextState.IF.PC.to_ulong() + signExtendedImm);
                        nextState.ID.nop = true;
                    } else {
                        nextState.IF.PC = bitset<32>(nextState.IF.PC.to_ulong() + signExtendedImm);
                        nextState.ID.nop = true;
                    }
                }

                // HALT instruction
                else if (opcode == bitset<7>("1111111")) {
                    nextState.IF.nop = true;
                    nextState.IF.PC = state.IF.PC;
                    nextState.ID.nop = true;
                }
            }

			
			
			
			/* --------------------- IF stage --------------------- */
          if (!nextState.IF.nop) {
                bool stall = nextState.ID.nop || nextState.EX.nop || nextState.ID.is_hazard;
                if (!stall) {
                    bitset<32> instruction = ext_imem.readInstr(nextState.IF.PC);
                    if (instruction == bitset<32>(string(32, '1'))) {
                        nextState.IF.nop = true;
                        nextState.ID.nop = true;
                    } else {
                        nextState.ID.Instr = instruction;
                        nextState.IF.PC = bitset<32>(nextState.IF.PC.to_ulong() + 4);
                        nextState.ID.nop = false;
                        nextState.ID.is_I_type = false;
                        nextState.ID.is_hazard = false;
                    }
                }
            }
    		nextState.WB.nop = nextState.MEM.nop;
    nextState.MEM.nop = nextState.EX.nop;
    nextState.EX.nop = nextState.ID.nop;
            
    // Check for halt condition
    if (nextState.IF.nop && nextState.ID.nop && nextState.EX.nop && 
        nextState.MEM.nop && nextState.WB.nop) {
        halted = true;
    }
     myRF.outputRF(cycle);
    printState(nextState, cycle);

			
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

         int32_t signExtend(string imm, int originalBits) {
        bitset<32> extended;
        for(int i = 0; i < imm.length(); i++) {
            extended[i] = (imm[imm.length()-1-i] == '1');
        }
        if(imm[0] == '1') {
            for(int i = imm.length(); i < 32; i++) {
                extended[i] = 1;
            }
            return -((-extended.to_ulong()) & ((1ull << originalBits) - 1));
        }
        return extended.to_ulong();
    }

         int detectHazard(bitset<5> rs) {
            if (rs == bitset<5>(0)) return 0;

            if (nextState.MEM.rd_mem && nextState.MEM.Wrt_reg_addr == rs&& nextState.MEM.wrt_enable) {
          
            return 3;  // Indicate stall needed
        }
        
        // Data hazard - forward from MEM
        if (nextState.MEM.Wrt_reg_addr == rs && nextState.MEM.wrt_enable && !nextState.MEM.nop) {
            return 1;  // Forward from MEM
        }
        
        // Data hazard - forward from WB
        if (nextState.WB.Wrt_reg_addr == rs && nextState.WB.wrt_enable &&!nextState.WB.nop) {
            return 2;  // Forward from WB
        }
        
        return 0;  // No hazard
        
         }
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
	DataMem dmem_fs = DataMem("FS", ioDir);

	
	FiveStageCore FSCore(ioDir, imem, dmem_fs);

    while (1) {
		
		
		if (!FSCore.halted)
			FSCore.step();

		if ( FSCore.halted)
			break;


    }
    
	
   
	dmem_ss.outputDataMem();
	dmem_fs.outputDataMem();

	return 0;

}
