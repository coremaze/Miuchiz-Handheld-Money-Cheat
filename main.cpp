#include <iostream>
#include <windows.h>

HANDLE OpenDrive(char* drive){
    HANDLE handle = CreateFile(drive,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_NO_BUFFERING | FILE_ATTRIBUTE_NORMAL,
                    NULL);
    return handle;
}

char ReadSectors(char buffer[], HANDLE handle, int sector, int numsectors){
    int location_start = sector * 512;
    int location_end = location_start + (numsectors * 512);
    int readsize = location_end - location_start;
    DWORD bytesread;
    for (int i=0; i<readsize; i++){
        buffer[i] = (BYTE)0; }

    SetFilePointer(handle, (long int)location_start, 0, FILE_BEGIN);
    ReadFile(handle, buffer, readsize, &bytesread, NULL);
    return (DWORD)buffer;

}

void WriteSectors(HANDLE handle, int sector, int numSectors, char data[]){
    int location_start = sector * 512;
    SetFilePointer(handle, (long int)location_start, 0, FILE_BEGIN);
    DWORD byteswritten;
    WriteFile(handle, data, numSectors * 512, &byteswritten, NULL);
}



bool IsHandheld(HANDLE handle){
    if ((signed int)handle == -1){
        return false;
    }
    char buffer[512];
    ReadSectors(buffer, handle, 0, 1);
    char marker[11] = { };
    for (int i=0; i<10; i++){
        marker[i] = buffer[43+i];
        if (!marker[i]){
            return false;
        }
    }
    return (bool)!strcmp(marker, "SITRONIXTM");
}

void SendCommand(HANDLE handle, char buffer[]){
    WriteSectors(handle, 0x31, 1, buffer);

}

void Eject(HANDLE handle){
    char command[512] = { 0x28, 0, 0, 2 };
    SendCommand(handle, command);
}

int FindHandhelds(HANDLE buffer[]){
    unsigned int mask = GetLogicalDrives();
    char letters[256] = { };
    int lettersindx = 0;
    for (int i = 0; mask; i++){
        if (mask&1){
            char letter = 'A' + i;
            letters[lettersindx] = letter;
            lettersindx++;
        }
        mask = mask>>1;
    }

    unsigned int bufferindx = 0;
    for (int i = 0; i<lettersindx; i++){
        char thisLetter[2] = { letters[i] };
        char drive[15] = { };

        sprintf(drive, "\\\\.\\%s:", thisLetter);
        HANDLE handle = OpenDrive(drive);
        if (IsHandheld(handle)){
            buffer[bufferindx] = handle;
            bufferindx++;
        }

    }

    return bufferindx;

}

int ReadPage(HANDLE handle, char buffer[], unsigned char bank, unsigned char page){
    char command1[512] = { 0x80 };
    SendCommand(handle, command1);


    char command2[512] = { 0x28, 0, 0, bank, page };
    SendCommand(handle, command2);

    char data[512*0x80] = {};
    ReadSectors(data, handle, 0x58, 0x80);

    for (int i = 4; i<0x1004; i++){
        buffer[i-4] = data[i];
    }

    char command3[512] = { 0x81 };
    SendCommand(handle, command3);

    return 0x1000;

}

void WritePage(HANDLE handle, char data[], unsigned int bank, unsigned int page){
    char command1[512] = { 0x80 };
    SendCommand(handle, command1);

    char command2[512] = { 0x2A, 0, 0, bank, page, 0, 0, 0x10, 0 };
    SendCommand(handle, command2);

    char buffer[0x2000] = {  };
    for (int i = 0; i<0x1000; i++){
        buffer[i] = data[i];
        buffer[i+0x1000] =  data[i];
    }
    WriteSectors(handle, 0x33, 16, buffer);

    char command3[512] = { 0x81 };
    SendCommand(handle, command3);
}

int main()
{

    HANDLE handhelds[128] = {};
    int num_handhelds = FindHandhelds(handhelds);
    if (num_handhelds < 1){
        std::cout << "No handhelds are connected.";
        Sleep(3000);
        return 0;
    }
    if (num_handhelds > 1){
        std::cout << "Multiple handhelds are connected";
        Sleep(3000);
        return 0;
    }
    HANDLE handheld = handhelds[0];
    char data[0x1000] = {};

    unsigned char character_bank = 1;
    unsigned char character_page = 0xFF;

    ReadPage(handheld, data, character_bank , character_page);

    //set creditz
    data[0x9AA] = 0x78;
    data[0x9AB] = 0x56;
    data[0x9AC] = 0x34;
    data[0x9AD] = 0x12;
    WritePage(handheld, data, character_bank, character_page);

    Eject(handheld);

    std::cout << "The handheld has been given 12,345,678 creditz.";

    Sleep(3000);

    return 0;
}
