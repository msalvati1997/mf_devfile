  

Multi-flow device file
==================

## Martina Salvati
Advanced Operating Systems - MS Degree in Computer Engineering - Academic Year 2021/202


## Project's description

This project is related to a Linux device driver implementing low and high priority flows of data. Through an open session to the device file a thread can read/write data segments. The data delivery follows a First-in-First-out policy along each of the two different data flows (low and high priority). After read operations, the read data disappear from the flow. Also, the high priority data flow must offer synchronous write operations while the low priority data flow must offer an asynchronous execution (based on delayed work) of write operations, while still keeping the interface able to synchronously notify the outcome. Read operations are all executed synchronously. The device driver should support 128 devices corresponding to the same amount of minor numbers.

  

## Installation

1. Clone this repo

```bash

cd

git clone https://github.com/msalvati1997/mf_devfile.git

```

2. Begin install

- The install script can be use in order to install/uninstall the module.

  

```bash

cd ~/driver

sudo ./install.sh

```
<p align="left" width="100%">
<img alt="" src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAA1EAAABsCAYAAACLiThEAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAALXRFWHRDcmVhdGlvbiBUaW1lAE1vbiAyMSBNYXIgMjAyMiAwNDowMjowMCBBTSBQRFR9RRvuAAAAGXRFWHRTb2Z0d2FyZQBnbm9tZS1zY3JlZW5zaG907wO/PgAAIABJREFUeAHtnQncVdP+/1cJaVBCpYuoe7uZQ+lmzJQSUm73lgwPEUUUSqFfUsRNiJehlPEVuRlehlSGhEYSuhmSoVApkajM+vdev/86v/2c9rDOc85znnO6n+/r9Txnn73XtN9r7X3Wd32/a61Ku++++0YjEQEREAEREAEREAEREAEREAER8CJQ2SuUAomACIiACIiACIiACIiACIiACFgCUqLUEERABERABERABERABERABEQgAwJSojKApaAiIAIiIAIiIAIiIAIiIAIiICVKbUAEREAEREAEREAEREAEREAEMiBQxTdsSUmJad++fSp4586dU8cc6Lr4qH3o+XAvBb0f9H50bUG/D/p9VP9A/QP1D9Q/cL8JmfYPXLxC/KzkuzpfrVq1TI0aNVL3sGzZstQxB7ouPmofej7cS0HvB70fXVvQ74N+H9U/UP9A/QP1D9xvQqb9AxevED+9lahCLLzKJAIiIAIiIAIiIAIiIAIiIAL5JqA5UfkmrvxEQAREQAREQAREQAREQASKmoCUqKKuPhVeBERABERABERABERABEQg3wSkROWbuPITAREQAREQAREQAREQAREoagJSooq6+lR4ERABERABERABERABERCBfBOQEpVv4spPBERABERABERABERABESgqAlIiSrq6lPhRUAEREAEREAEREAEREAE8k1ASlS+iSs/ERABERABERABERABERCBoiYgJaqoq0+FFwEREAEREAEREAEREAERyDeBKvnOMNf5PfDAA2b8+PHmxRdfLJV0pUqVzIQJE8ztt99uWrZsaQ4//PBS1/lyzz33mBdeeMGceeaZplWrVqZXr16bhXEndtttN3PGGWeYvffe25D2p59+au69917zxRdfmMqVK5vTTz/dHHPMMaZatWrmo48+MmPGjDFffvmladOmjbnwwgtt+e6++26b3J///Gfzr3/9y3z++eemT58+5rLLLostnytD2OcVV1xhDj300LBL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w1gKLgIiIAIiIAIiIAIisKUSqLKl3tiWcl904A899NDI2zn//PPNN998E3k9kwuk8+qrr2YSxTts9erVzcMPP2z69etnPvnkE+94BOzevbsNf++994bGS7oeGintZDblS0tKXyuAQDHXX+/evU3Y4MDpp59uST7yyCP2GUeR/+OPP+y5u+66y0ybNs08/vjjZrvttjOEPeSQQ8z2229vvv76a/Pmm2/a5y2T98eNN95o9txzT3PVVVeVekbPPPNM06pVK9OrV6/Qmk26HhpJJ0VABERABESgyAlIiSrwCnzooYfME088YUt5zDHHmKOOOsoMHjw4VervvvsudZztweeff27uvvvubJNRfBEQgQwIPProo+a5554zxx57rDn88MPNkCFDbOyff/7ZbLvttvZ4hx12sErSnDlzNku5Z8+epmHDhmb06NFm9erVpkGDBjYsAX3fH6RPGq+88oqNm+lAx2aF0gkREAEREAER2MIJSInKsoIPO+wws//++5sXXnih1Ohtlsmmoq9atSp1/O2339qR6M8++yx1joPKlSubM844w7Ru3dpUq1bNfPTRRwarzRdffFEqXNSXffbZxwwdOtRe/uWXX0yXLl1KBWU0m/NVqlQxuPz98MMPNv233347FY6RcEbTa9asaUfNn3nmGTN16lSz44472rAu4IgRI9yhYQR7/fr1Ns3TTjvN7L777mbrrbc2ixYtMvfff79ZunRpKmx5HfiUj7y32mor69J35JFHmrVr1252/yeddJI58cQTDZ3RL7/80jz44INm4cKFXsVm5H/Dhg2mVq1apnHjxmbNmjVmzJgx5r333kvFh1WjRo3M7NmzDawI+8EHH9gOd1L9b7PNNtaK0LJlS4PSTd1gwbzwwguNa19x6VPnUfVD27n88svNxx9/bMuHwn/cccdZi8gNN9xg0ttq6obSDuL4xbU/3/qLS5+ixN1/WlEjv9aoUcMqPZlahlF8+KNufv/991Bm8+bNM+3atTPpSlSlSpWs0jNq1Cgzf/58WzYGQ1w4V79ciHp/cA0r1vvvv28tWLxLUOwkIiACIiACIiAC0QQ0JyqajdeVTz/91NBJHTZsmEFBaNOmjalatapX3FwFOvnkkw1WqjvvvNO6y6GYDBw40CpXPnnQWe/UqVNKkQqLg0vhSy+9ZM466yzr8nfRRRel0keRbNu2rbnt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a0/5TqbZUkjpPkSgnAiwTxTLut93333llEPpZFl84JprrtlsP7DSofRNBERABERABERABEQgioAsUVFkdF4EthACzDHbddddrYsWc5xw7cN9SyICIiACIiACIiACIlA2AlKiysZNsUSgaAiwUAV7JV1yySV2wQH2s3rggQeKpvwqqAiIgAiIgAiIgAgUGgG58xVajag8IiACIiACIiACIiACIiACBU2gckGXToUTAREQAREQAREQAREQAREQgQIjICWqwCqkmItz4IEHpjbsLKT7GDdunGHFQf6Cq6C5MiZdd+HK+smeWazaJxEBERABERABERABEdi7TWdbAAAYM0lEQVQyCGhOVIHXIx1w9hyKkvPPP9/uCRV1PZ/n2ZuqvJZ7r169umHJ9X79+plPPvkko9vq3r27DX/vvfeGxku6Hhop7WQ25UtLSl8rgEAx1x+DA3fddZdhPzAnEyZMMA8++KB55ZVXzCOPPGLfESjybu82wk+bNs08/vjjdmuEsMEFt8R6Uny2VSAsy6Wz4TCbDbOfFc9rJu8vtlpgjyxWqww+42yF0KpVK9OrVy93e6U+k66XCqwvIiACIiACIpAjAlKicgSyvJJ56KGH7L5JpH/MMceYo446yu734/JL33DWna+ITzZzvfvuuysia+UpAiIQQ4B9wlBy5syZs1ko9hF77rnnzLHHHms3Cx4yZIgNw0bLrOaIxMXv2bOnadiwoRk9erTdOLtBgwY2L+L5vr9InzRQ+ihnUIkiHYkIiIAIiIAIFBoBKVFZ1shhhx1mN0llA8zy+OFftWpVqoTffvutHUn+7LPPUuc4qFy5st1os3Xr1qZatWqGTTCxurDppo8wWsxmm3SCkJo1a9pR7Msvv9yQF9d/+eUXu8EtLnuEJf23337bhmdj3KFDh9pjwnXp0sUeu39J8QnHSDaj4eSNReuZZ54xU6dONTvuuKPNy6U1YsQId5jarJcysWz3pj3PDCvRLVq0yNx///12E9FU4HI68CkfWbMZL5aAI4880m54G+TH9ZNOOsmceOKJtrP65ZdfWv4LFy7kUqIwcr9hwwZTq1Yt07hxY7NmzRozZswY895776XiMlrfqFEjM3v2bMuKsB988IGhw5zUfrbZZhtrBWjZsqVBaadusIBeeOGFxrXPuPTj6oe2Qztjg2jKx0bLbByLReOGG26w7S91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0nFj1TiULibh07HmGJk4caJ14cHNDobMwSAMo9LMI/IR3PKYrI4rJQtkMILuOq5J8XORf1IecdeT+BA36v5QXrA+YeHD5YkR/m7duoVmF1V/LnDSdRcu/TOp/L7tKz3dTL4z/xAXMPYXYqU72lZwTlUUP588ctE+4vLPRfo+94HLXFkWlqB8LVu2tIvVPP300/b90KxZMztnkncBguUvfQ4oi4TgZolFnoVXeObdEuco/O79xMqWM2fOtOkxCMAqnrwDgs8+ClV6+rx/UNAkIiACIiACIlARBKREZUmdzVXLa4NV36KxYALWJ+Zh0dmgM4yrXJgLW1iadMQHDx5slzhnlT4EK1qwE8o5Fixg7hKT/7H4uFW+kvKnM0Unik4cnVvSHjlyJElaoZNFJwxFjmXIGZ1nEYqgJcGFjftk882STXPXWOkvWL64OFzLNn9Y48LphInvCItl4AaaJEl8XPyo+2OuC2zbtWtnXb7YVyzMshdVfy79pOsuXPpnUvl921d6uu67D19Wj9t///1N165dbbthUAPLWlCi+AXDhB1n2z5cmlH55yp9l0+uP3kOWUae553VLRkQYXAluAw58zKDzzRlQKknPO2xatWq5pxzzrHvJ86zmqkb5EARO+644+xACkoUC6QMHz7cfrp74flKT59ny23I68LpUwREQAREQATyRaDSpr1lKnYyTb7uVPmIgAhskQTo4GPhuO+++7bI+9NNiYAIiIAIiIAIFB4BzYkqvDpRiURABERABERABERABERABAqYgJSoAq4cFU0EREAEREAEREAEREAERKDwCMidr/DqRCUSAREQAREQAREQAREQAREoYAKyRBVw5ahoIiACIiACIiACIiACIiAChUdASlTh1YlKJAIiIAIiIAIiIAIiIAIiUMAEpEQVcOWoaCIgAiIgAiIgAiIgAiIgAoVHQEpU4dWJSiQCIiACIiACIiACIiACIlDABKREFXDlqGgiIAIiIAIiIAIiIAIiIAKFR0BKVOHViUokAiIgAiIgAiIgAiIgAiJQwASkRBVw5ahoIiACIiACIiACIiACIiAChUegim+RSkpKTPv27VPBO3funDrmQNfFR+1Dz4d7Kej9oPejawv6fdDvo/oH6h+of6D+gftNyLR/4OIV4qf3Zru1atUyNWrUSN3DsmXLUscc6Lr4qH3o+XAvBb0f9H50bUG/D/p9VP9A/QP1D9Q/cL8JmfYPXLxC/PRWogqx8CqTCIiACIiACIiACIiACIiACOSbgOZE5Zu48hMBERABERABERABERABEShqAlKiirr6VHgREAEREAEREAEREAEREIF8E5ASlW/iyk8EREAEREAEREAEREAERKCoCUiJKurqU+FFQAREQAREQAREQAREQATyTeD/Ab0CLUetYuFkAAAAAElFTkSuQmCC" />
</p> 

## Multiflow driver

  

| Driver file operations - fops |
|---|
| static int dev_open(struct inode *, struct file *);
| static int dev_release(struct inode *, struct file *);
| static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);
| static long dev_ioctl(struct file *filp, unsigned int command, unsigned long arg);


## IOCTL - device file settings

The device parameters of files can be manipulated by the ioctl() system call.

Some macros have been created that make it easier to set parameters (driver/multiflow-driver_ioctl.h)

| MACRO | DESCRIPTION |
|---|---|
| IOCTL_RESET | Allows to set the default setting parameters of the device file.| | | |
| IOCTL_HIGH_PRIO | Allows to set the workflow to high priority. | 
| IOCTL_LOW_PRIO| Allows to set the workflow to low priority.| 
| IOCTL_BLOCKING | Allows to set the working mode operation to non blocking. | 
| IOCTL_NO_BLOCKING | Allows to set the working mode operation to blocking.| 
| IOCTL_SETTIMER| Allows to set the timer for blocking operation. | 
| IOCTL_ENABLE | Allows to set the device state to enable. | 
| IOCTL_DISABLE | Allows to set the device state to disable. | 

  
  

## A simple program-flow

  

1. Open device :

```bash

fd = open(device,O_RDWR|O_APPEND);

```

2. When open a device of multiflow-driver the files operation called is

```

static int dev_open(struct inode *, struct file *);

```

- The operation creates structured session data with default settings.

3. The IOCTL'S MACRO can be used in order to set the device file parameters.

Example:

```

ioctl(fd, IOCTL_RESET);

```

4. After setting the parameters, a write or read operation can be called.

Example:

```

##Example of write operation

char * buff = malloc(sizeof(char)*7);

write(fd,buff,strlen(buff));

  

##Example of read operation

char * buff = malloc(sizeof(char)*7);

read(fd,buff,7);

```

  

## Testing
