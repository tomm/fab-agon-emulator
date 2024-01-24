#ifndef __EOS_H_
#define __EOS_H_

#define ESC 0x1b
#define CTRL_W 0x17 // 23 decimal, MOS escape code, ElectronOS 8 bits ASCII value
#define CTRL_Z 0x1a

bool eos_wakeup ();

#endif // __EOS_H_