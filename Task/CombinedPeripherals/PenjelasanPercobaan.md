Program ini adalah demo RTOS di ESP32-S3 yang menjalankan beberapa perpheral secara paralel di dua core.

Core 1 menghandle input tombol, rotary encoder, potensiometer, dan beberapa output servo, dan OLED display. 
Core 0 menangani stepper motor dan LED indikator.

Fungsinya:
- Tombol 1 (atas) merupakan enable stepper.
- Tombol 2 (bawah) mengatur arah putar stepper.
- Encoder mengatur target posisi motor.
- Potensiometer mengatur sudut servo.
- OLED menampilkan status sistem (pot, servo, posisi, arah, status).
- LED1 terus berkedip (hearbeat) 
- LED2 menandakan on/off dari stepper.