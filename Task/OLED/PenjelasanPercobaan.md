Program merupakan demo ESP32 untuk menjalankan dua OLED sekaligus. 

Kedua Task 1 dan 2 bersamaan mengatur OLED dengan alamat I2C berbeda untuk menampilkan counter dan grafis berupa bar yang bergerak maju mundur.

Tujuannya adalah menunjukkan bahwa dua layar I²C bisa bekerja bersamaan dengan dua core ESP32, masing-masing menjalankan tugas tampilan serupa secara independen dan simultan.

dalam video dapat dilihat OLED berhasil menampilkan display bersamaan.