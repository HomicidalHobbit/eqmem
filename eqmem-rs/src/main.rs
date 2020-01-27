#![allow(dead_code)]

use libc::c_void;
use std::marker::PhantomData;
use std::mem;
use std::{thread, time};

#[link(name = "eqmem", kind = "static")]
extern "C" {
    fn LocalAllocate(size: usize, tag: i32) -> *mut c_void;
    fn Deallocate(ptr: *mut c_void);
    fn SetLocalLogging(enable: bool);
}

fn main() {
    println!("Hello, world!");
    let t = thread::spawn(move || {
        let secs = time::Duration::from_secs(5);
        thread::sleep(secs);
        unsafe {
            SetLocalLogging(true);
            let ptr = LocalAllocate(1024 * 1024 * 128, 0);
            Deallocate(ptr);
        }
    });
    unsafe {
        SetLocalLogging(true);
        let ptr = LocalAllocate(1024, 0);
        t.join().unwrap();
        Deallocate(ptr);
    }
    let mut b = BinVec::<usize>::with_capacity(1024, 0);
    b.push(0)
}

struct BinVec<T> {
    phantom: PhantomData<T>,
}

impl<T> BinVec<T> {
    fn new(_bin: usize) -> Vec<T> {
        let v = Vec::<T>::new();
        // TODO: inform allocator that we have been created.
        v
    }
    fn with_capacity(size: usize, _bin: usize) -> Vec<T> {
        unsafe {
            let ptr: *mut c_void = LocalAllocate(mem::size_of::<T>() * size, 0);
            Vec::<T>::from_raw_parts(ptr as *mut T, 0, 1)
        }
    }
}
