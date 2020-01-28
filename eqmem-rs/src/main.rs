#![allow(dead_code)]

use libc::c_void;
use std::alloc::{GlobalAlloc, Layout};
use std::marker::PhantomData;
use std::mem;
use std::{thread, time};

#[global_allocator]
static A: EQMem = EQMem;

#[link(name = "eqmem", kind = "static")]
extern "C" {
    fn LocalAllocate(size: usize, tag: i32) -> *mut c_void;
    fn Deallocate(ptr: *mut c_void);
    fn Reallocate(ptr: *mut c_void, size: usize) -> *mut c_void;
    fn SetLocalLogging(enable: bool);
    fn Malloc(size: usize) -> *mut c_void;
    fn Free(ptr: *mut c_void);
    fn Realloc(ptr: *mut c_void, new_size: usize) -> *mut c_void;
}

fn main() {
    println!("Hello, world!");
    /*
    let t = thread::spawn(move || {
        let secs = time::Duration::from_secs(5);
        thread::sleep(secs);
        unsafe {
            SetLocalLogging(true);
            let ptr = LocalAllocate(1024 * 1024 * 128, 0);
            Deallocate(ptr);
        }
    });
    */
    unsafe {
        SetLocalLogging(true);
        let ptr = LocalAllocate(1024, 0);
        //    t.join().unwrap();
        Deallocate(ptr);
    }
    let mut _b = BinVec::<usize>::with_capacity(1024, 0);
    //b.push(0);
}

struct BinVec<T> {
    phantom: PhantomData<T>,
}

impl<T> BinVec<T> {
    fn new(_bin: usize) -> Vec<T> {
        let v = Vec::<T>::new();
        // TODO: inform allocator that we have been created.
        println!("Inner V: {:?}", &v as *const Vec<T>);
        v
    }
    fn with_capacity(size: usize, _bin: usize) -> Vec<T> {
        unsafe {
            let ptr: *mut c_void = LocalAllocate(mem::size_of::<T>() * size, 0);
            Vec::<T>::from_raw_parts(ptr as *mut T, 0, size)
        }
    }
}

pub struct EQMem;

unsafe impl GlobalAlloc for EQMem {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        //Malloc(layout.size()) as *mut u8
        LocalAllocate(layout.size(), 0) as *mut u8
    }

    unsafe fn dealloc(&self, ptr: *mut u8, _layout: Layout) {
        //Free(ptr as *mut c_void);
        Deallocate(ptr as *mut c_void);
    }

    unsafe fn realloc(&self, ptr: *mut u8, _layout: Layout, new_size: usize) -> *mut u8 {
        //Realloc(ptr as *mut c_void, new_size) as *mut u8
        Reallocate(ptr as *mut c_void, new_size) as *mut u8
    }
}
