const express = require('express');
const bodyParser = require('body-parser');
const midtransClient = require('midtrans-client');
const path = require('path');


const app = express();
app.use(bodyParser.json());
app.use(bodyParser.urlencoded({ extended: true }));

//=======================================
//Halaman Utama
//=======================================
app.get('/', (req, res) => {
    // res.sendFile(path.join(__dirname, 'index.html'));
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
    // path.join(__dirname, '../public/index.html')
});

// Koneksi ke Midtrans (Ganti dengan Server Key Sandbox Anda)
let snap = new midtransClient.Snap({
    isProduction: false, // Set false untuk mode uji coba (Sandbox)
    // serverKey: 'SB-Mid-server-YOUR_ACTUAL_SANDBOX_KEY_HERE' 
    serverKey: 'SB-Mid-server-mmu8tI0L2SaFsWLHj_Rd6alR' 
});

// Database sementara untuk menyimpan status slot (0 = mati, 1 = aktifkan)
// let slotStatus = {
//     slot1: 0,
//     slot2: 0
// };

// ==========================================
// 1. ENDPOINT UNTUK USER: MEMBUAT NOTA & QRIS
// ==========================================
// Database sementara diperbarui untuk menyimpan milidetik durasi aktif
let slotStatus = {
    slot1: 0, // Jika aktif, ini akan berisi durasi dalam milidetik (misal: 3600000)
    slot2: 0
};



app.get('/order-slot/:slotNumber', async (req, res) => {
    const slot = req.params.slotNumber;
    // Membaca durasi dari parameter URL (default ke 1 jam jika kosong)
    const jam = parseInt(req.query.durasi) || 1; 
    
    if (slot !== '1' && slot !== '2') {
        return res.status(400).send("Slot tidak valid.");
    }

    // Hitung harga dinamis: Rp 5.000 per jam
    const hargaPerJam = 5000;
    const totalHarga = hargaPerJam * jam;

    // Masukkan info durasi ke dalam Order ID agar terbaca saat callback sukses
    const orderId = `SLOT-${slot}-JAM-${jam}-${Date.now()}`;

    let parameter = {
        "transaction_details": {
            "order_id": orderId,
            "gross_amount": totalHarga
        },
        "item_details": [{
            "id": `SLOT-${slot}`,
            "price": hargaPerJam,
            "quantity": jam,
            "name": `Sewa Slot ${slot} selama ${jam} Jam`
        }],
        "enabled_payments": ["gopay", "shopeepay", "qris"]
    };

    try {
        const transaction = await snap.createTransaction(parameter);
        res.send(`
            <body style="font-family:sans-serif; text-align:center; padding-top:50px;">
                <h2>Total Pembayaran: Rp ${totalHarga.toLocaleString('id-ID')}</h2>
                <p>Slot ${slot} untuk durasi ${jam} Jam</p>
                <a href="${transaction.redirect_url}" target="_blank" style="background:#28a745; color:white; padding:15px 25px; text-decoration:none; border-radius:5px; font-weight:bold;">BAYAR QRIS</a>
            </body>
        `);
    } catch (error) {
        res.status(500).send("Gagal membuat QRIS: " + error.message);
    }
});

// ==========================================
// 2. ENDPOINT WEBHOOK: DIHUBUNGI MIDTRANS SAAT LUNAS
// ==========================================
//app.post('/midtrans-callback', (req, res) => {
    app.post('/api/payment/callback', (req, res) => {


    const notification = req.body;

    const orderId = notification.order_id;
    const transactionStatus = notification.transaction_status;
    const fraudStatus = notification.fraud_status;

    if (transactionStatus === 'settlement' && fraudStatus === 'accept') {
        // Deteksi slot mana yang dibayar berdasarkan Order ID (contoh: SLOT-1-171829...)
        if (orderId.includes('SLOT-1')) {
            slotStatus.slot1 = 1;
            console.log("Slot 1 Sukses Dibayar! Siap dinyalakan oleh ESP32.");
        } else if (orderId.includes('SLOT-2')) {
            slotStatus.slot2 = 1;
            console.log("Slot 2 Sukses Dibayar! Siap dinyalakan oleh ESP32.");
        }
    }

    res.status(200).send('OK');
});

// ==========================================
// 3. ENDPOINT UNTUK ESP32: CEK STATUS PEMBAYARAN
// ==========================================
// app.get('/api/cek_status', (req, res) => {
//     // Mengirim status JSON ke ESP32
//     res.json(slotStatus);
// });

app.get('/api/cek_status', (req, res) => {
    res.set('Cache-Control', 'no-store');
    res.status(200).json(slotStatus);
});

// ==========================================
// 4. ENDPOINT UNTUK ESP32: RESET STATUS JIKA SUDAH NYALA
// ==========================================
app.get('/api/reset_status', (req, res) => {
    const slot = req.query.slot;
    if (slot === '1') {
        slotStatus.slot1 = 0;
    } else if (slot === '2') {
        slotStatus.slot2 = 0;
    }
    res.send("Reset Berhasil");
});

console.log('__dirname:', __dirname);
console.log(
    'index:',
    path.join(__dirname, 'public', 'index.html')
);

// Jalankan Server di Port 3000
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
    console.log(`Server Backend berjalan di port ${PORT}`);
});
