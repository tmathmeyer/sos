
    kprintf("%5bs%5bi\n", "translate(0)          = ", translate_address((void *)0));
    kprintf("%5bs%5bi\n", "translate(1)          = ", translate_address((void *)1));
    kprintf("%5bs%5bi\n", "translate(4096)       = ", translate_address((void *)4096));
    kprintf("%5bs%5bi\n", "translate(2097152)    = ", translate_address((void *)2097152));
    kprintf("%5bs%5bi\n", "translate(629145600)  = ", translate_address((void *)629145600));
    kprintf("%5bs%5bi\n", "translate(1073741824) = ", translate_address((void *)1073741824));
    kprintf("%5bs%5bi\n", "translate(1073741823) = ", translate_address((void *)1073741823));
	
    page_t src = 1073741824;
    page_t des = 1084227584;

    frame_t frame = map_page(containing_address(src), 0, &falloc);
    map_page_to_frame(containing_address(des), frame, 0, &falloc);
    kprintf("%5bs%5bi\n", "allocated frame = ", frame);
    kprintf("%5bs%5bi\n", "mapped page to  = ", translate_address((void *)src));
    kprintf("%5bs%5bi\n", "mapped page to  = ", translate_address((void *)des));

    char *c_src = (char *)src;
    char *c_des = (char *)des;

    memcpy(c_src, "TEST", 5);
    kprintf("String is = %4es\n", c_src);
    kprintf("String is = %4es\n", c_des);
