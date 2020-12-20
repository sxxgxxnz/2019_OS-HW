#include "Page.h"

// Set Flag to Page Entry
void kSetPageEntryData(PTENTRY *pstEntry, DWORD dwUpperBaseAddress, DWORD dwLowerBaseAddress, DWORD dwLowerFlags, DWORD dwUpperFlags)
{
    pstEntry->dwAttributeAndLowerBaseAddress = dwLowerBaseAddress | dwLowerFlags;
    pstEntry->dwUpperBaseAddressAndEXB = (dwUpperBaseAddress & 0xFF) | dwUpperFlags;
}

void kInitializePageTables(void)
{
    PML4TENTRY *pstPML4TEntry;
    PDPTENTRY *pstPDPTEntry;
    PDENTRY *pstPDEntry;
    PTENTRY *pstPTEntry;
    DWORD dwMappingAddress;
    int i;
    DWORD targetAddress = 0x1ff000;

    //Create Table PML4
    // PML4 테이블 생성
    // 첫 번째 엔트리 외에 나머지는 모두 0으로 초기화
    pstPML4TEntry = (PML4TENTRY *)0x100000;
    kSetPageEntryData(&(pstPML4TEntry[0]), 0x00, 0x101000, PAGE_FLAGS_DEFAULT, 0);
    for (i = 1; i < PAGE_MAXENTRYCOUNT; i++)
    {
        kSetPageEntryData(&(pstPML4TEntry[i]), 0, 0, 0, 0);
    }

    //Create Table Directory Pointer
    // 페이지 디렉터리 포인터 테이블 생성
    // 하나의 PDPT로 512GByte까지 매핑 가능하므로 하나로 충분함
    // 64개의 엔트리를 설정하여 64GByte까지 매핑함
    pstPDPTEntry = (PDPTENTRY *)0x101000;
    for (i = 0; i < 64; i++)
    {
        kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0x104000 + (i * PAGE_TABLESIZE), PAGE_FLAGS_DEFAULT, 0);
    }
    for (i = 64; i < PAGE_MAXENTRYCOUNT; i++)
    {
        kSetPageEntryData(&(pstPDPTEntry[i]), 0, 0, 0, 0);
    }


    //Create Table Page Directory
    // 페이지 디렉터리 테이블 생성
    // 하나의 페이지 디렉터리가 1GB까지 매핑 가능
    // 여유있게 64개의 페이지 디렉터리를 생성하여 총 64GB까지 지원
    pstPDEntry = (PDENTRY *)0x104000;
    dwMappingAddress = 0; 
    for (i = 0; i < PAGE_MAXENTRYCOUNT * 64; i++)
    {
        // Double Mapping
        // 첫 번째 페이지 디렉터리 테이블이 새로 만들어지는
        // 페이지 테이블을 가리키도록 한다.
        if (i == 0 || i == 5)
        {
            kSetPageEntryData(&(pstPDEntry[i]), 0, 0x142000,
                              PAGE_FLAGS_DEFAULT, 0);
            dwMappingAddress += PAGE_DEFAULTSIZE;
            continue;
        }
        // 32비트로는 상위 어드레스를 표현할 수 없으므로, MB 단위로 계산한 다음
        // 최종 결과를 다시 4KB로 나누어 32비트 이상의 어드레스를 계산함
        kSetPageEntryData(&(pstPDEntry[i]), (i * (PAGE_DEFAULTSIZE >> 20)) >> 12,
                          dwMappingAddress, PAGE_FLAGS_DEFAULT | PAGE_FLAGS_PS, 0);
        dwMappingAddress += PAGE_DEFAULTSIZE;
    }

    // Create Page Table
    // 페이지 테이블 하나 생성
    // 하나의 페이지 테이블은 2MB까지 매핑한다.
    // 0x000000 번지부터 0x200000 번지 까지 매핑해준다.
    // 우리가 접근해야하는 0x1FF000 번지의 경우 읽기전용으로 만들어준다.
    pstPTEntry = (PTENTRY *)0x142000;
    dwMappingAddress = 0;
    for (i = 0; i < PAGE_MAXENTRYCOUNT; i++)
    {
        
        //page table read only
        if (dwMappingAddress == 0x1FF000)
        {
            kSetPageEntryData(&(pstPTEntry[i]), 0,
                              dwMappingAddress, PAGE_FLAGS_P, 0);
            dwMappingAddress += 0x1000;
            continue;
        }

        kSetPageEntryData(&(pstPTEntry[i]), 0,
                          dwMappingAddress, PAGE_FLAGS_DEFAULT, 0);
        dwMappingAddress += 0x1000;
    }
}
