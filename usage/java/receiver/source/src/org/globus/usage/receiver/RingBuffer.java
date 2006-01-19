/*
 * Copyright 1999-2006 University of Chicago
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.globus.usage.receiver;

import org.globus.usage.packets.CustomByteBuffer;

/*I don't think this class exists yet in the java platform.
  It's pretty trivial to implement one.
Since one thread will be writing into this while another thread reads
out, it must be thread-safe.*/

public class RingBuffer {

    private CustomByteBuffer[] queue;
    private int numObjects, maxObjects;
    private int inputIndex, outputIndex;

    public RingBuffer(int capacity) {
        maxObjects = capacity;
        numObjects = 0;
        queue = new CustomByteBuffer[maxObjects];
        inputIndex = outputIndex = 0;
    }

    /*Returns and removes next object (FIFO) if there is one;
      if ringbuffer is empty, returns null.*/
    public synchronized CustomByteBuffer getNext() {
        try {
            while (numObjects == 0) {
                wait();
            }
        } catch (InterruptedException e) {
            return null;
        }
        
        CustomByteBuffer theNext;
        theNext = queue[outputIndex];
        queue[outputIndex] = null;
        outputIndex = (outputIndex + 1) % maxObjects;
        numObjects --;
        return theNext;
    }

    /*Returns true if insert was successful, false if ringbuffer 
      was already full and the insert failed.*/
    public synchronized boolean insert(CustomByteBuffer newBuf) {
        if (numObjects == maxObjects) {
            return false;
        } else {
            queue[inputIndex] = newBuf;
            inputIndex = (inputIndex + 1) % maxObjects;
            numObjects ++;
            notify();
            return true;
        }
    }

    /*These query methods are synchronized so that they can't be called when
      the other thread is halfway through inserting or removing, which might
      give the wrong answer.*/
    public synchronized boolean isFull() {
        return numObjects == maxObjects;
    }

    public synchronized boolean isEmpty() {
        return numObjects == 0;
    }

    public synchronized int getCapacity() {
        return maxObjects;
    }
    
    public synchronized int getNumObjects() {
        return numObjects;
    }

    /*JUnit tests*/

}
