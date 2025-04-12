# Sword of Secrets
Unlock the hidden secrets of the sword. What kind of secrets lie in waiting? Designed for security researchers and hardware enthusiasts, the Sword of Secrets is your ultimate mystery in digital exploration.

<p align = "center">
    <img src="https://raw.githubusercontent.com/gili-yankovitch/SwordOfSecrets/main/images/front.png" height="500"> <img src="https://raw.githubusercontent.com/gili-yankovitch/SwordOfSecrets/main/images/back.png" height="500">
</p>

### About The Project
Sword of Secrets is a hardware hacking challenge. It is suitable for both novice and experienced tinkerers and hackers. It utilizes a low number of components to make the challenge easier to grasp, but hold in itself enough intricacies that will require the challenger a keen eye and sharp mind to solve all challenges.

To solve the challenge you will need to discover a series of flags, hidden inside the sword. Each one will help you unlock the next step in the challenge, to finally reveal the ultimate secret the sword holds.

## How to Solve
You, as the challenger, will face  a couple of tricks on the hardware level. It is strongly recomended to identify which chips are used, find their datasheet and understand how they are connected. Then, dive into the software to understand how it is built and figure out what needs to be done to make the sword uncover its truth to you. All the software (except the encryption keys and the flags) is available here on this repository. It is recommended to try and reverse engineer the PCB before heading towards the `hw` folder.

### Source structure
* `aes.cpp` [AES implementation](https://github.com/kokke/tiny-AES-c)
* `main.cpp` Boot and Setup

... The rest is up to you. :-)

### You Will Need
The following list is a recommended list of equipment that is suggested to solve the challenge:
* Multimeter
* Soldering Iron
* Jumper wires
* An external microcontroller (Any Arduino/RPi)

### Software
Please install the following software to be able to flash software to the device:
* [ch32fun](https://github.com/cnlohr/ch32fun/wiki/Installation) Follow install guide for toolchain and `minichlink` flash utility
* [Flash Update](https://swordofsecrets.com/update.html) Use online update tool to update the sword using preflashed bootloader

### Hardware
All PCB specs are provided here under `hw/`. This can be easily manufactured as well as paneled for a larger volume. There are no special requirements for this board's manufacturing process.

#### Bill of Materials
| Quantity | Component Description | Datasheet |
|----------|-----------------------|-----------|
| 1 | CH32V003 RISC-V Microcontroller | [Datasheet](http://mcu.cz/images_articles/5007-CH32V003Reference-Manual.PDF) |
| 1 | AMS1117-3.3 LDO Voltage Regulator | [Datasheet](https://www.ti.com/lit/gpn/lm1117) |
| 1 | CH340N USB-toSerial | [Datasheet](https://cdn.sparkfun.com/assets/5/0/a/8/5/CH340DS1.PDF) |
| 1 | W25Q128JVSIQ 128MB NOR Flash | [Datasheet](https://www.mouser.co.il/datasheet/2/949/w25q128jv_revf_03272018_plus-1489608.pdf) |
| 1 | SMD 10 kOhms 0805 Resistor|  |
| 1 | SMD 100 kOhms 0805 Resistor |  |
| 3 | SMD 0.1 uF 0805 Capacitor |  |


<!-- LICENSE -->
## License

Distributed under the MIT License.

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- CONTACT -->
## Contact

Gili Yankovitch - [@GiliYankovitch](https://x.com/GiliYankovitch) - giliy@nyxsecurity.net

Project Link: [https://gili-yankovitch.github.io/SwordOfSecrets](https://gili-yankovitch.github.io/SwordOfSecrets)

<!-- MARKDOWN LINKS & IMAGES -->
<!-- https://www.markdownguide.org/basic-syntax/#reference-style-links -->
[front]: images/front.png
[back]: images/back.png
