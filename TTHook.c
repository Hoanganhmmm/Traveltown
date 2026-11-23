/*
 * TTHook.c — dylib hook cho Travel Town iOS (build bằng Xcode)
 * ------------------------------------------------------------
 * Cách làm: vá BẢNG CON TRỎ METHOD (vùng DATA), không patch code (không cần JIT)
 * -> chạy được với Apple ID miễn phí, không cần jailbreak, không cần paid account.
 *
 * Các hằng số dưới đây được trích từ phân tích tĩnh UnityFramework (2.12.1831),
 * vmaddr == file offset (vì __TEXT có vmaddr = 0).
 *
 *  - methodPointers array (module "TravelTown.Main.dll")  @ 0x8aa94b8
 *  - CanMerge (token 0x6001222, idx 4641)                 @ 0x8ab25c0
 *  - CanMergeSpecificItem (token 0x6000a0d, idx 2572)     @ 0x8aae518
 *
 *  >>> LƯU Ý QUAN TRỌNG (trung thực):
 *  Vá con trỏ method chỉ chặn được lời gọi GIÁN TIẾP (virtual/interface/reflection/delegate).
 *  Nếu game gọi CanMerge bằng lệnh BL TRỰC TIẾP (thường có khi cùng assembly), cách này
 *  KHÔNG chặn được. Khi đó chỉ còn cách patch code (cần quyền JIT) — Apple ID miễn phí
 *  KHÔNG có JIT, nên không làm được. Đây là giới hạn thật, không phải lỗi code.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <mach-o/dyld.h>
#include <mach/mach.h>
#include <unistd.h>

/* trampoline: luôn trả true, bỏ qua mọi đối số */
static bool hook_return_true(void) {
    return true;
}

/* tìm base address của UnityFramework lúc chạy (sau ASLR slide) */
static uintptr_t unity_base(void) {
    uint32_t n = _dyld_image_count();
    for (uint32_t i = 0; i < n; i++) {
        const char *name = _dyld_get_image_name(i);
        if (name && strstr(name, "UnityFramework") != NULL) {
            return (uintptr_t)_dyld_get_image_header(i);
        }
    }
    return 0;
}

/* ghi con trỏ vào bảng method (data). vm_protect phòng trường hợp page read-only. */
static void patch_ptr(uintptr_t entry, void *new_fn) {
    uintptr_t page = entry & ~(uintptr_t)(getpagesize() - 1);
    vm_protect(mach_task_self(), (vm_address_t)page, (vm_size_t)getpagesize(),
               FALSE, VM_PROT_READ | VM_PROT_WRITE);
    *(void **)entry = new_fn;
    vm_protect(mach_task_self(), (vm_address_t)page, (vm_size_t)getpagesize(),
               FALSE, VM_PROT_READ);
}

__attribute__((constructor))
static void tthook_init(void) {
    uintptr_t base = unity_base();
    if (!base) {
        return;   /* không tìm thấy UnityFramework -> bỏ qua, không crash */
    }

    patch_ptr(base + 0x8ab25c0, (void *)&hook_return_true);   /* CanMerge          -> merge any items */
    patch_ptr(base + 0x8aae518, (void *)&hook_return_true);   /* CanMergeSpecificItem -> merge specific */
}
