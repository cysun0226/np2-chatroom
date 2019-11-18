# who
- broadcast message of user pipe in server 3
- server 2 user pipe
- server 2 env var

  
* np_multi_proc 與 np_single_proc 的差異在於:
  - np_multi_proc 在 client 建立連線時就 fork 出一隻 process 來處理這個 client ，不同 client 連線的對象是 server 的不同 process，所以無法使用原本的 pipe 來實做 user pipe (必須在 fork 以前就建立 pipe)
  - np_multi_proc 必須可以讓不同的 process 之間拿到其他 client 的資訊以及要傳送的訊息。

因此

 - np_multi_proc 使用 FIFO 來實做 user pipe
 - np_multi_proc 使用 shared memory 來儲存 client 的資訊以及要傳送的訊息
 - np_multi_proc 需要使用 signal


 # FIFO

 When you open a FIFO for writing, the writer is blocked until there is a reader.

 You are probably missing the reader.

 You cannot write to a pipe, then close it, and then have the reader come along later. Such storage semantics is accomplished by using a regular file.
