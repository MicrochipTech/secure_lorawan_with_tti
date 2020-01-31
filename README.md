# secure_lorawan_with_tti
> “Wireless Made Fun!" - Secure Authentication with SAMR34 & ATECC608A and The Things Industries’s Join Server

<p>
<a href="https://www.microchip.com/design-centers/security-ics/trust-platform/trust-go/trust-go-lora-secure-authentication-with-join-servers" target="_blank">
<img border="0" alt="HaveFun" src="Doc/HaveFun.png" width="800">
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp
</p>
</a>

**Welcome to the Microchip Workshop we did at The Things Conference 2020 in Amsterdam. From here you have the choice to either follow the Lab manual instructions described below or to follow the Microchip_workshop_slide.pdf document which expose you on a real situation with use cases**
</br>

1. [Abstract](#step1)
2. [Prerequisites](#step2)
3. [Resources](#step3)
4. [Hardware Setup](#step4)
5. [Lab 1](#step5)
6. [Lab 2](#step6)
7. [Running the demo](#step7)


## Abstract <a name="step1"></a>

With hands-on approach, you will be guided through the process of getting started with developing a secure LoRa® end device product using Microchip Technology’s pre-provisioned ATECC608A secure element along with TTI Join server.

## Prerequisites <a name="step2"></a>

- <a href="https://www.thethingsindustries.com/technology/hardware#gateway" target="_blank">The Things LoRa(r) Gateway</a></br>
- <a href="https://www.microchip.com/Developmenttools/ProductDetails/DM320111" target="_blank">SAM R34 Xplained Pro Evaluation Kit</a></br>
- <a href="https://www.microchipdirect.com/product/search/all/AT88CKSCKTUDFN-XPRO" target="_blank">CryptoAuthentication UDFN Socket Kit</a></br>
- <a href="https://www.microchipdirect.com/product/search/all/ATECC08A-TNGLORA" target="_blank">ATECC608A-TNGLORA Secure Element</a><br>
- Device security (manifest) file. You can obtain this from your <a href="https://www.microchipdirect.com/orders" target="_blank">Microchip Direct order history</a></br>
- Access to The Things Industries Cloud Hosted. <a href="cloud@thethingsindustries.com" target="_blank">Contact The Things Industries to get onboarded</a></br>
- An application in The Things Industries Cloud Hosted. <a href="https://enterprise.thethingsstack.io/v3.3.2/guides/getting-started/console/#create-application" target="_blank">See instructions</a></br>
- Your The Things Industries Join Server address. <a href="https://enterprise.thethingsstack.io/v3.3.2/guides/cloud-hosted/tti-join-server/" target="_blank">See Join Server addresses</a></br>

- Download and install Atmel Studio 7.0 IDE. </br>
https://www.microchip.com/mplab/avr-support/atmel-studio-7

- Open Atmel Studio 7.0 IDE. </br>
- Then, you need Advanced Software Framework (ASFv3) v3.47.0 release or upper release. </br>
Install ASFv3 as an extension to Atmel Studio from the menu: Tools -> Extensions and Updates …
- Once the installation is complete, you must restart Atmel Studio. </br>
- Download and install a serial terminal program like Tera Term. </br>
https://osdn.net/projects/ttssh2/releases/

Note: ASFv3 is an MCU software library providing a large collection of embedded software for AVR® and SAM flash MCUs and Wireless devices. ASFv3 is configured by the ASF Wizard in Atmel Studio 7.0 (installed as an extension to Studio). ASFv3 is also available as a standalone (.zip) with the same content as Studio extension (https://www.microchip.com/mplab/avr-support/advanced-software-framework).

Important:
Until the next Atmel Studio IDE release, you have to manually install the Device Part Pack for developing with SAMR34/R35 on Atmel Studio 7.0 IDE.
(all products released in between IDE releases of Atmel Studio should be manually added by user to develop applications).
- Go to Tools -> Device Pack Manager </br>
- Check for Updates </br>
- Search for SAMR34 and click install </br>
- Repeat the same for SAMR35 </br>
- Restart Atmel Studio 7.0 IDE </br>


## Resources <a name="step3"></a>

Several accounts have been already created specifically for this workshop in the microchip tenant.</br>
An appendix sheet with the credentials has been provided during the workshop.</br>

Useful links:</br>
- <a href="https://microchip.eu1.cloud.thethings.industries/console" target="_blank">TTI Network server</a></br>
- <a href="https://microchip.join.cloud.thethings.industries/" target="_blank">TTI Join server</a></br>
- <a href="https://enterprise.thethingsstack.io/v3.3.2/guides/" target="_blank">The Things Stack Guides</a></br>

TTI and Microchip developed a security solution for LoRaWAN that enables secure key provisioning and secure cryptographic operations using secure elements.
</br>
- <a href="https://www.thethingsindustries.com/technology/security-solution" target="_blank">End-to-end LoRaWAN Security solution</a></br>
- <a href="https://enterprise.thethingsstack.io/v3.3.2/guides/claim-atecc608a/" target="_blank">Claim ATECC608A Secure Elements</a></br>
- <a href="https://enterprise.thethingsstack.io/v3.3.2/guides/cloud-hosted/tti-join-server/activate-devices-cloud-hosted/" target="_blank">Activate devices on the Things Industries Cloud Hosted</a></br>



## Hardware setup <a name="step4"></a>

Configure the DIP switch of the CryptoAuthenticationUDFN Socket kit for I2C communication with the host microcontroller.
**1, 3 and 6 must be placed to ON position**
</br>
![](Doc/DIP_Switch.png)
</br>

Open the socket board
</br>
![](Doc/OpenSocketBoard.png)
</br>

Make sure the ATECC608A device is ready to be inserted in the right direction.
Make sure the pin 1 of the component (represented by a point) is located at the bottom left.
</br>
![](Doc/SecureElement.png)
</br>

Place the Secure Element in the UDFN socket
</br>
![](Doc/SecureElementPlacement.png)
</br>

Make sure the Secure Element is properly seated and the pin 1 is located at the bottom left.
</br>
![](Doc/SecureElementPlaced.png)
</br>
Close the clam shell lid.
</br>

Attach the CryptoAuthenticationUDFN Socket kit to the SAM R34 Xplained Pro board on the **EXT3 header.**
</br>
Plug the antenna.
Attach a USB cable to SAM R34 Xplained Pro board's EDBG micro-B port on the right.</br>
The USB ports powers the board and enables the user to communicate with the kits.
</br>
![](Doc/FullSetup.png)


- Wait for USB driver installation and COM ports mounting. </br>
- Launch Tera Term program and configure the serial ports mounted with: **115200 bps, 8/N/1**

## Lab 1 <a name="step5"></a>

1. Open the <a href="https://microchip.eu1.cloud.thethings.industries/console" target="_blank">TTI Network Server Console</a></br>
2. Login by using the TTI Credentials from the appendix sheet</br>
![](Doc/lab1_ns_login.png) </br>
3. Select “Go to applications”</br>
![](Doc/lab1_goto_applications.png)</br>
4. Select “thethingsconference” application
![](Doc/lab1_thethingsconference_application.png)</br>
5. Go to “Devices” in the left menu and click on “+ Add Device” to reach the end device registration page.</br>
a) Fill the device ID (use the appendix sheet)</br>
b) Fill the MAC Version with: MAC V1.0.2</br>
c) Fill the PHY Version with: PHY V1.0.2 REV B</br>
d) Fill the Frequency Plan: Europe 863-870 MHz</br>
e) Scroll to the lower part of the device registration page and make sure that “Over the Air Activation (OTAA)” is selected</br>
f) Fill the JoinEUI (AppEUI in LoRaWAN versions before 1.1) (use the appendix sheet)</br>
g) Fill the DevEUI (use the appendix sheet)</br>
h) Uncheck External Join Server</br>
i) Fill the AppKey (use the appendix sheet)</br>
j) Click “Create Device”</br>
</br>Exemples:</br>
![](Doc/lab1_device_registration.png)</br>
![](Doc/lab1_device_activation.png)</br>
6. You will now reach the device overview page of your device. The end device should now be able to join the private network</br>
![](Doc/lab1_device_overview.png)</br>
7. Plug the antenna and always make sure you have the antenna plugged to your SAMR34 Xpro board before powering it up</br>
8. Connect your SAMR34 Xpro board to the computer through the micro-USB cable. USB cable must be connected to the EDBG USB connector of the kit</br>
9. Wait for USB driver installation and COM port mounting. The USB port powers the board and enables the user to communicate with the kit</br>
![](Doc/lab1_hardware_setup.png)</br>
10. Open Serial Console program (e.g. TeraTerm) </br>
11. Configure Terminal setup</br>
![](Doc/lab1_terminal_setup.png)</br>
12. Configure Serial port setup, COMxx 115200 bps / 8 / N / 1</br>
![](Doc/lab1_serial_setup.png)</br>
13. Restart the application by pressing Reset button on SAMR34 Xpro board</br>
14. From the console, press ‘1’ to Select Lab1</br>
![](Doc/lab1_select_lab.png)</br>
15. Manually provision your device by entering:</br>
* DevEUI </br>
* JoinEUI </br>
* AppKey </br>
Use the appendix sheet to find your OTAA credentials.</br>
16. Press ‘1’ to confirm the provisioning</br>
![](Doc/lab1_serial_provisioning.png)</br>
17. Enter your first name (10char max.) and press enter</br>
![](Doc/lab1_enter_first_name.png)</br>
18. Your device should successfully join the network</br>
![](Doc/lab1_join_network.png)</br>
19. Press SAMR34 Xpro SW0 button to transmit an uplink message</br>
![](Doc/lab1_transmit_uplink_message.png)</br>
20. Observe the result on the dashboard and confirm you can see your data</br>
![](Doc/lab1_dashboard.png)</br>



## Lab 2 <a name="step6"></a>

Activating the device within TTI is the next step.</br>

Go here: <a href="https://www.thethingsindustries.com/technology/security-solution" target="_blank">https://www.thethingsindustries.com/technology/security-solution</a>

And follow the steps to get started with claiming your devices.


## Running the demo <a name="step7"></a>

Go back to the Tera Term UART console
</br>
![](Doc/UART_Console2.png)
</br>
Press "1" to start the Demo Application
</br>
Select the band where your device is operating
</br>
![](Doc/UART_Console3.png)
</br>
Then, the end device application transmits a Join Request message. If a Join Accept message was received and validated, the SAM R34 Xplained Pro board will be joined to the Join Server.
</br>
![](Doc/UART_Console4.png)
</br>
Press "2" to send a packet consisting of a temperature sensor reading
</br>
![](Doc/UART_Console5.png)
</br>

