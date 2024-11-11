<a id="readme-top"></a>
<div align="center">
  <h1 style = "font-size: 3rem;
    text-transform: uppercase;
    background: linear-gradient(45deg, #1F51FF, #B026FF);
    -webkit-background-clip: text;
    background-clip: text;
    color: transparent;
    text-shadow: 0 0 10px rgba(0, 243, 255, 0.3);
    animation: glowPulse 3s infinite;">Sword of Secrets</h1>
</div>

<!-- ABOUT THE PROJECT -->
## About The Project

![front] ![back]

Sword of Secrets is a hardware hacking challenge. It is suitable for both novice and experienced tinkerers and hackers. It utilizes a low number of components to make the challenge easier to grasp, but hold in itself enough intricacies that will require the challenger a keen eye and sharp mind to solve all challenges.

To solve the challenge you will need to discover a series of flags, hidden inside the sword. Each one will help you unlock the next step in the challenge, to finally reveal the ultimate secret the sword holds.

## How to Solve
You, as the challenger, will face  a couple of tricks on the hardware level. It is strongly recomended to identify which chips are used, find their datasheet and understand how they are connected. Then, dive into the software to understand how it is built and figure out what needs to be done to make the sword uncover its truth to you. All the software (except the encryption keys and the flags) is available here on this repository. It is recommended to try and reverse engineer the PCB before heading towards the `hw` folder.

## Source structure
* `aes.cpp` [AES implementation](https://github.com/kokke/tiny-AES-c)
* `main.cpp` Boot and Setup
* `rdprot.cpp` CH32V003 Internal flash read protection
* `secboot.cpp` Setup and quest implementation

## You Will Need
The following list is a recommended list of equipment that is suggested to solve the challenge:
* Multimeter
* Soldering Iron
* Jumper wires
* An external microcontroller (Any Arduino/RPi)

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
