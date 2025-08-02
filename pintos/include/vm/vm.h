#ifndef VM_VM_H
#define VM_VM_H
#include <stdbool.h>
#include "threads/palloc.h"
#include "lib/kernel/hash.h"


enum vm_type {
	/* page not initialized*/
	VM_UNINIT = 0,
	/* page not related to the file, aka anonymous page*/
	VM_ANON = 1,
	/* page that realated to the file */
	VM_FILE = 2,
	/* page that hold the page cache, for project 4 */
	VM_PAGE_CACHE = 3,

	/* Bit flags to store state */

	/* Auxillary bit flag marker for store information. You can add more
	 * markers, until the value is fit in the int. */
	VM_MARKER_0 = (1 << 3),
	VM_MARKER_1 = (1 << 4),

	/* DO NOT EXCEED THIS VALUE. */
	VM_MARKER_END = (1 << 31),
};

#include "vm/uninit.h"
#include "vm/anon.h"
#include "vm/file.h"
#ifdef EFILESYS
#include "filesys/page_cache.h"
#endif

struct page_operations;
struct thread;

#define VM_TYPE(type) ((type) & 7)

/* The representation of "page".
 * This is kind of "parent class", which has four "child class"es, which are
 * uninit_page, file_page, anon_page, and page cache (project4).
 * DO NOT REMOVE/MODIFY PREDEFINED MEMBER OF THIS STRUCTURE. 
객체 지향 프로그래밍의 상속을 모방하여 설계
 */
struct page {
	const struct page_operations *operations; //페이지 연산을 위한 함수 테이블
	void *va;              /* Address in terms of user space */
	struct frame *frame;   /* Back reference for frame */

	struct hash_elem hash_elem; //hash table elem 
	

	/* Per-type data are binded into the union.
	 * Each function automatically detects the current union */
	union {
		struct uninit_page uninit; //초기화 전 페이지
		struct anon_page anon; //익명 페이지
		struct file_page file; //파일 페이지
#ifdef EFILESYS
		struct page_cache page_cache; //프로젝트4
#endif
	};
};

/* The representation of "frame" 
물리 메모리의 한 페이지(프레임)
*/
struct frame {
	void *kva; //커널 가상 주소(Kernel Virtual Address)
	struct page *page; //이 물리 프레임에 현재 매핑되어 있는 page 구조체에 대한 포인터
};

/* The function table for page operations.
 * This is one way of implementing "interface" in C.
 * Put the table of "method" into the struct's member, and
 * call it whenever you needed. */
struct page_operations {
	//스왑 공간에 저장된 페이지를 물리 메모리로 다시 가져오는(swap-in) 작업을 수행
	bool (*swap_in) (struct page *, void *);
	//물리 메모리의 페이지를 스왑 공간으로 내보내는(swap-out) 작업을 수행
	bool (*swap_out) (struct page *);
	//페이지와 관련된 모든 자원을 해제하고 정리하는 함수
	void (*destroy) (struct page *);
	enum vm_type type;
};

#define swap_in(page, v) (page)->operations->swap_in ((page), v)
#define swap_out(page) (page)->operations->swap_out (page)
#define destroy(page) \
	if ((page)->operations->destroy) (page)->operations->destroy (page)

/* Representation of current process's memory space.
 * We don't want to force you to obey any specific design for this struct.
 * All designs up to you for this. 
 우리는 이 구조체에 대해 특정 설계를 따르도록 강요하지 않습니다. 모든 설계는 당신에게 달려있습니다.
 */
struct supplemental_page_table {
};


// 주어진 page 구조체의 가상 주소(va)를 기반으로 해시 값을 반환한다.
unsigned
page_hash (const struct hash_elem *elem, void *aux UNUSED){

	const struct page *p = hash_entry(elem, struct page, hash_elem);
	return hash_bytes (&p->va, sizeof p->va);
};


// page1의 가상 주소가 page2의 가상 주소보다 작으면 true를 반환한다. => page1이 더 "앞선다"
bool
page_less (const struct hash_elem *elem1, const struct hash_elem *elem2,
			void *aux UNUSED){
	const struct page *page1 = hash_entry(elem1, struct page, hash_elem);
	const struct page *page2 = hash_entry(elem2, struct page, hash_elem);

	return page1->va < page2->va;
}




#include "threads/thread.h"

void supplemental_page_table_init (struct supplemental_page_table *spt);
bool supplemental_page_table_copy (struct supplemental_page_table *dst,
		struct supplemental_page_table *src);
void supplemental_page_table_kill (struct supplemental_page_table *spt);

struct page *spt_find_page (struct supplemental_page_table *spt,
		void *va);
bool spt_insert_page (struct supplemental_page_table *spt, struct page *page);
void spt_remove_page (struct supplemental_page_table *spt, struct page *page);

void vm_init (void);
bool vm_try_handle_fault (struct intr_frame *f, void *addr, bool user,
		bool write, bool not_present);

#define vm_alloc_page(type, upage, writable) \
	vm_alloc_page_with_initializer ((type), (upage), (writable), NULL, NULL)
bool vm_alloc_page_with_initializer (enum vm_type type, void *upage,
		bool writable, vm_initializer *init, void *aux);
void vm_dealloc_page (struct page *page);
bool vm_claim_page (void *va);
enum vm_type page_get_type (struct page *page);

#endif  /* VM_VM_H */
