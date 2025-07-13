// ProcessWnd.h
#pragma once
#include "interface.h"

class ProcessWnd : public ImguiWnd {
public:
    explicit ProcessWnd(Context* ctx);
    void Render(bool* p_open = nullptr) override;
    void RenderProcessWnd();
    void RenderMemWnd(DWORD pid);
private:

    char processIdText_[16] = "1234";   // ����ID�����
    char addressText_[16] = "00401000"; // ��ַ�����
    char sizeText_[16] = "256";         // ��С�����
    char writeDataText_[1024] = "";     // д�������ı���
    
    // �ڴ洰��״̬
    bool showMemoryWindow_ = false;
    DWORD targetPid_ = 0;

    std::vector<PROCESSENTRY32> processList;
};
