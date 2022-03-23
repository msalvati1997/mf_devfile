
# TEST

- [TEST](#test)
- [Execution of test](#execution-of-test)
  - [Test 1 : simple test](#test-1--simple-test)
  - [Test 2 : Write and read test](#test-2--write-and-read-test)
  - [Test 3 : Concurrency test](#test-3--concurrency-test)
  - [Test 4 : Timeout test](#test-4--timeout-test)
  - [Test 5 : Enable Disable test](#test-5--enable-disable-test)
  - [Test 6 : Param test](#test-6--param-test)


# Execution of test

To run the tests start the execution of the bash script ./test.sh

```bash
cd ~/test
sudo ./test.sh
```
A dialog box will open - it  will guide the user through the selection of command inputs.

## Test 1 : Simple test 
  This is a simple test in which a user tries a write and a read operation in different streams (high prio and low prio) and different session. 

## Test 2 : Write and read test 
  This is a test in which a user tries a write and read in the same session and different streams. 

## Test 3 : Concurrency test
  This is a test in which different users try writes and reads in different streams and different sessions. 

## Test 4 : Timeout test
This is a test in which different users try writes and reads operation setting the timeout's param to nsec. 

## Test 5 : Enable Disable test
This is a test in which a user tries writes and reads operation. Later disable the device through the IOCTL's command.  After disabling, it tries to writes and reads operation again. Another users try writes and reads operation after the disabling of device. Operations of already open sessions are expected to be allowed, while operations of new sessions fail.
## Test 6 : Param test 
This is a test in which different users tries writes and reads operation. During the program flow, the cat command is called for each parameter:  

```bash
cat /sys/module/multiflow_driver/parameters/##param 
```
