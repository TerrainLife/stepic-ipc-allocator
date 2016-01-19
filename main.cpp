#include <iostream>
#include <cstring>

struct MBnode
{
    char *begin;
    char *end;      // next free byte after mem block

    MBnode *prev;
    MBnode *next;
};
struct MBlist
{
	MBnode *firstNode;
	unsigned int count;
	
	MBlist()
	{
		firstNode = NULL;
		count = 0;
	}
};

class SmallAllocator {
private:
		static const unsigned int MEM_SIZE = 1048576;
        char Memory[MEM_SIZE];
        MBlist list;
public:
        SmallAllocator()
        {
            std::memset(Memory, 0, sizeof(Memory));
        }

        void *Alloc(unsigned int Size) {
            unsigned int needSz = Size + sizeof(MBnode);
            MBnode *pMB = list.firstNode;
            
            if(pMB == NULL)		// No nodes
            {
                if( MEM_SIZE > needSz )
                {
                	fillNode(Memory, Size);
					MBnode *insMB = insertNode(NULL, NULL, (MBnode *)Memory);

                    return (void *)(insMB->begin);				// ret memory
                }	
                return NULL;
            }
            
            if(isSizeEnough(NULL, pMB, needSz))      	// check on size between first block & start of memory
            {
            	fillNode(Memory, Size);
            	MBnode *insMB = insertNode(NULL, pMB, (MBnode *)Memory);

                return (void *)(insMB->begin);				// ret memory
            }
            
            for(; pMB->next != NULL; pMB = pMB->next)
            {
                if(isSizeEnough(pMB, pMB->next, needSz))      	// check on size between this & next block
                {
                	fillNode(pMB->end, Size);
					MBnode *insMB = insertNode(pMB, pMB->next, (MBnode *)pMB->end);

                    return (void *)(insMB->begin);				// ret memory
                }
            }

            if(isSizeEnough(pMB, NULL, needSz))      	// check on size between last block & end of memory
            {
            	fillNode(pMB->end, Size);
            	MBnode *insMB = (MBnode *)pMB->end;
				insertNode(pMB, pMB->next, insMB);

                return (void *)(insMB->begin);				// ret memory
            }

            return NULL;
        }
        
        void *ReAlloc(void *Pointer, unsigned int Size) {
        	if(Pointer == NULL) return Alloc(Size);
        	if(Size == 0) { Free(Pointer); return NULL; }
			for(MBnode *pMB = list.firstNode; pMB != NULL; pMB = pMB->next)
            {
            	if(pMB->begin == Pointer)
            	{
            		void *ret = NULL;
            		// std::cout << "Realloc " << (unsigned int)Pointer << "  size " << Size << std::endl;
            		if( (pMB->end - pMB->begin) >= Size )
            		{
            			// std::cout << "Realloc smaller or eq data" << (unsigned int)Pointer << "  size " << Size << std::endl;
            			
            		 	pMB->end = pMB->begin + Size;
            			ret  = (void *)(pMB->begin);
            		}
					else
					{
						// std::cout << "Realloc bigger data" << (unsigned int)Pointer << "  size " << Size << std::endl;
						
						unsigned int needSz = Size + sizeof(MBnode);
						// if(isSizeEnough(pMB, pMB->next, needSz))	// check on size until next block - Can we just extend this block?
						
						// std::cout << "Realloc1 " << (unsigned int)((char*)pMB->next - pMB->begin) << "  size " << needSz << std::endl;
						if( ( (char*)pMB->next - pMB->begin) > needSz)
						{
							// std::cout << "Realloc2 " << std::endl;
							pMB->end = pMB->begin + Size;
							ret = (void *)(pMB->begin);
						}
						else							// need to realloc mem block
						{
							ret = Alloc(Size);			// TODO: memory intersection handling!!!
							if(ret) 
							{
								MBnode *newMB = (MBnode *) ((char*)ret - sizeof(MBnode));
								memcpy( newMB->begin, pMB->begin, (pMB->end - pMB->begin) );
								deleteNode(pMB);
								
								// std::cout << "Realloc3 2 " << (unsigned int)ret << "  newMB = " << (unsigned int)newMB << std::endl;
								
							}
						}
					}

					return ret;
            	}
            	// std::cout << "Realloc NO FOUND " << (unsigned int)pMB->begin << "  " << (unsigned int)Pointer << " first node = "<< (unsigned int)list.firstNode->begin << std::endl;
            }
            return NULL;
        }
        void Free(void *Pointer) {
        	if(Pointer == NULL) return;
			
			for(MBnode *pMB = list.firstNode; pMB != NULL; pMB = pMB->next)
            {
            	if(pMB->begin == Pointer)
            	{
            		deleteNode(pMB);
            		return;
            	}
            }
        }

private:
        //list methods
        bool isSizeEnough(MBnode *beg, MBnode *end, unsigned int needSize)
        {
        	char *prevEnd = NULL;
        	if(beg == NULL)
        		prevEnd = Memory;
        	else
        		prevEnd = beg->end;
        	if(end == NULL) end = (MBnode *)(&Memory[MEM_SIZE]);		// last node before end check
            return ((char *)end - prevEnd) > needSize;
        }

        MBnode *insertNode(MBnode *before, MBnode *after, MBnode *ins)
        {
        	list.count++;
        	if(before == NULL)
        		list.firstNode = ins;
			ins->prev 	 = before;
			ins->next	 = after;
			if(before) 	before->next = ins;
			if(after)	after->prev  = ins;
			
			return ins;
        }     

        void deleteNode(MBnode *del)
        {
        	if(del == NULL) return;
        	
        	list.count--;
        	if(del->prev == NULL)
        		list.firstNode = del->next;
			if(del->prev) del->prev->next = del->next;
			if(del->next) del->next->prev = del->prev;
        }

        void fillNode(char *node, unsigned int Size)
        {
			MBnode *fillNode = (MBnode *)node;
			fillNode->begin  = node + sizeof(MBnode);
			fillNode->end 	 = fillNode->begin + Size;
        }
};

int main()
{
    SmallAllocator A1;
    int * A1_P1 = (int *) A1.Alloc(sizeof(int));
    A1_P1 = (int *) A1.ReAlloc(A1_P1, 2 * sizeof(int));
    A1.Free(A1_P1);
    
    SmallAllocator A2;
    int * A2_P1 = (int *) A2.Alloc(10 * sizeof(int));
    for(unsigned int i = 0; i < 10; i++) A2_P1[i] = i;
    for(unsigned int i = 0; i < 10; i++) if(A2_P1[i] != i) std::cout << "ERROR 1" << std::endl;
    
    int * A2_P2 = (int *) A2.Alloc(10 * sizeof(int));
    for(unsigned int i = 0; i < 10; i++) A2_P2[i] = -1;
    for(unsigned int i = 0; i < 10; i++) if(A2_P1[i] != i) std::cout << "ERROR 2" << std::endl;
    
    for(unsigned int i = 0; i < 10; i++) if(A2_P2[i] != -1) std::cout << "ERROR 3" << std::endl;
    A2_P1 = (int *) A2.ReAlloc(A2_P1, 20 * sizeof(int));
    for(unsigned int i = 10; i < 20; i++) A2_P1[i] = i;
    for(unsigned int i = 0; i < 20; i++) if(A2_P1[i] != i) std::cout << "ERROR 4" << std::endl;
    
    std::cout << "A2_P1 = " << (unsigned int)A2_P1 << std::endl;
    std::cout << "A2_P2 = " << (unsigned int)A2_P2 << std::endl;
    
    for(unsigned int i = 0; i < 10; i++) if(A2_P2[i] != -1) std::cout << "ERROR 5" << std::endl;
    // std::cout << "A2_P1 REALLOC " << std::endl;
    A2_P1 = (int *) A2.ReAlloc(A2_P1, 5 * sizeof(int));
    std::cout << "A2_P1 = " << (unsigned int)A2_P1 << std::endl;
    for(unsigned int i = 0; i < 5; i++) if(A2_P1[i] != i) std::cout << "ERROR 6" << std::endl;
    for(unsigned int i = 0; i < 10; i++) if(A2_P2[i] != -1) std::cout << "ERROR 7" << std::endl;
    A2.Free(A2_P1);
    A2.Free(A2_P2);

    return 0;
}
