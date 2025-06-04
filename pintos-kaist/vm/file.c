/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "threads/vaddr.h"
#include "userprog/process.h"
#include "threads/mmu.h"

static bool file_backed_swap_in(struct page *page, void *kva);
static bool file_backed_swap_out(struct page *page);
static void file_backed_destroy(struct page *page);

static bool lazy_load_file(struct page *page, void *aux);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void vm_file_init(void)
{
	/* 전역 자료구조 초기화 */
	/** TODO: mmap 리스트 초기화
	 * mmap으로 만들어진 페이지를 리스트로 관리하면
	 * munmmap 시에 더 빠르게 할 수 있을 듯 합니다
	 *
	 */
}

// /* Initialize the file backed page */
bool file_backed_initializer(struct page *page, enum vm_type type, void *kva)
{
	/* Set up the handler */
	page->operations = &file_ops;

	struct file_page *file_page = &page->file;
	/** TODO: file 백업 정보를 초기화
	 * file_page에 어떤 정보가 있는지에 따라 다름
	 */
	struct file_info *aux = (struct file_info *)page->uninit.aux;
	file_page->aux = aux;

	return true;
}

/* 파일에서 내용을 읽어와 페이지를 스왑인합니다. */
static bool
file_backed_swap_in(struct page *page, void *kva)
{
	struct file_page *file_page UNUSED = &page->file;
	// swap_in을 위한 버퍼
	void *buffer[PGSIZE];
	/** TODO: 파일에서 정보를 읽어와 kva에 복사하세요
	 * aux에 저장된 백업 정보를 사용하세요
	 * file_open과 read를 사용하면 될 것 같아요
	 * 파일 시스템 동기화가 필요할수도 있어요
	 * 필요시 file_backed_initializer를 수정하세요
	 */
}

/* 페이지의 내용을 파일에 기록(writeback)하여 스왑아웃합니다. */
static bool
file_backed_swap_out(struct page *page)
{
	struct file_page *file_page UNUSED = &page->file;
	/** TODO: dirty bit 확인해서 write back
	 * pml4_is_dirty를 사용해서 dirty bit를 확인하세요
	 * write back을 할 때는 aux에 저장된 파일 정보를 사용
	 * file_write를 사용하면 될 것 같아요
	 * dirty_bit 초기화 (pml4_set_dirty)
	 */
}

/* 파일 기반 페이지를 소멸시킵니다. PAGE는 호출자가 해제합니다. */
static void
file_backed_destroy(struct page *page)
{
	struct file_page *file_page = &page->file;
	struct file_info *aux=file_page->aux;
	uint32_t read_bytes = aux->read_bytes;
	uint32_t zero_bytes = aux->zero_bytes;
	off_t offset=aux->ofs;
	/** TODO: dirty_bit 확인 후 write_back
	 * pml4_is_dirty를 사용해서 dirty bit 확인
	 * write back을 할 때는 aux에 저장된 파일 정보를 사용
	 * file_write를 사용하면 될 것 같아요
	 */
	if(pml4_is_dirty(thread_current()->pml4, page->va)){
		
		lock_acquire(&filesys_lock);
		file_seek(file_page,offset);
		file_write(file_page, page->frame->kva, offset);
		lock_release(&filesys_lock);
		pml4_set_dirty(thread_current()->pml4, page->va, 0);
	}
	file_close(file_page);
	free(aux);
}

/* Do the mmap */
/*
파일의 길이가 PGSIZE의 배수가 아니면 마지막 페이지는 일부만 유효하고,
나머지 바이트는 0으로 초기화.
*/
void *
do_mmap(void *addr, size_t length, int writable,
		struct file *file, off_t offset)
{
	// aux에 넣어줄 정보
	size_t zero_byte = PGSIZE - length;
	struct file_info *aux = malloc(sizeof(struct file_info));
	ASSERT(aux!=NULL);
	
	aux->file=file_reopen(file);
	aux->ofs=offset;
	aux->upage=addr;
	aux->read_bytes=length;
	aux->zero_bytes=zero_byte;
	aux->writable=writable;


	// TODO: 지연 로딩 함수 포인터 집어넣기
	if(!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_file, aux)) //<<어떻게 lazy_load_segemnt를 집어넣지?
		return NULL;
	return addr;
}

static bool 
lazy_load_file(struct page *page, void *aux){

	struct file_info *fi=aux;
	struct file *file=fi->file;
	off_t ofs = fi->ofs;
	uint8_t *kva = page->frame->kva;
	size_t page_read_bytes =  fi->read_bytes - (page->va - fi->upage);
	size_t page_zero_bytes = PGSIZE - page_read_bytes;

	bool success=false;
	//파일에서 데이터 읽기 
	file_seek(file, ofs);
	int test=file_read(file, kva, page_read_bytes);
	if(test==(int)page_read_bytes){
		memset(kva+page_read_bytes, 0, page_zero_bytes);
		success=true;
	}

	free(aux);
	//성공
	return success;
}

/* Do the munmap */
/* 언매핑시 0으로 채워진 부분은 파일에 반영하지 않아야 함.*/
void do_munmap(void *addr)
{

	
}
