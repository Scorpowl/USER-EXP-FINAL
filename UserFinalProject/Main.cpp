// main.cpp
#include "icbytes.h"   // Bu dosyanýn projenizde olmasý lazým
#include "ic_media.h" // BU DOSYANIN PROJENÝZDE OLMASI LAZIM! C1083 hatasýnýn sebebi bu.
#include "icb_gui.h"   // Bu dosyanýn projenizde olmasý lazým

#include <vector>
#include <string>
#include <numeric>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <algorithm> // std::min ve std::max için (E0040 hatasý için önemli)

#ifndef M_PI, min
#define M_PI 3.14159265358979323846
#undef min
#endif

// --- Global Deðiþkenler ve Yapýlar ---
struct PieSliceInfo {
    std::string label;
    double value;
    double percentage;
    double start_angle_deg;
    double end_angle_deg;
    unsigned int color;

    // Baþlatýlmamýþ üye uyarýsýný gidermek için varsayýlan constructor
    PieSliceInfo() : value(0.0), percentage(0.0), start_angle_deg(0.0), end_angle_deg(0.0), color(0U) {}
};

// GUI için global deðiþkenler
int FRM_PieChart_Display_Handle;    // Grafik resminin gösterileceði çerçevenin kulpu
ICBYTES pie_chart_image_global_obj; // Oluþturulan grafik resmini tutan ICBYTES nesnesi

// --- Fonksiyon Prototypleri ---
void CreatePieChart(ICBYTES& img, const std::vector<PieSliceInfo>& slices_param,
    const char* chart_title, int image_width, int image_height,
    int center_x, int center_y, int radius,
    unsigned int backcolor = 0xFFFAFAFA, unsigned int textcolor = 0xFF000000);

void GenerateAndDisplayPieChart_Main_GUI(const std::vector<std::pair<std::string, double>>& input_raw_data, const char* dynamic_title);
void TestPieChartWithData1();
void TestPieChartWithData2();


// --- CreatePieChart Fonksiyonu Implementasyonu ---
void CreatePieChart(ICBYTES& img, const std::vector<PieSliceInfo>& slices_param,
    const char* chart_title, int image_width, int image_height,
    int center_x, int center_y, int radius,
    unsigned int backcolor, unsigned int textcolor) {

    int top_margin_for_title = 30;
    int legend_color_box_size = 15;
    int legend_item_spacing_y = 25;
    int legend_initial_x_offset = 30;
    int legend_initial_y_offset = 20;

    CreateImage(img, image_width, image_height, ICB_UINT); // ICB_UINT 32-bit renk için
    img = backcolor; // Arka plan rengini ICBYTES atama operatörü ile ayarla

    // Grafik Baþlýðýný Çiz
    if (chart_title && strlen(chart_title) > 0) {
        size_t title_len = strlen(chart_title);
        int title_len_px = static_cast<int>(title_len * 12); // Ortalama harf geniþliði 12px varsayýmý
        int title_x_pos = (image_width - title_len_px) / 2;  // Baþlýðý resmi ortala
        if (title_x_pos < 5) title_x_pos = 5; // Kenara çok yapýþmasýn
        // Baþlýðý dikeyde marjýn ortasýna yerleþtir (Impress12x20 font yüksekliði ~20px)
        Impress12x20(img, title_x_pos, (top_margin_for_title - 20) / 2, chart_title, textcolor);
    }

    // Veri yoksa mesaj göster ve çýk
    if (slices_param.empty()) {
        Impress12x20(img, 10, top_margin_for_title + 10, "Pasta grafik icin veri yok.", textcolor);
        return;
    }

    // Dilimleri Çiz
    for (const auto& slice : slices_param) {
        // Yelpaze Yöntemi ile Basit Dolgu:
        double start_rad_fill = slice.start_angle_deg * M_PI / 180.0;
        double end_rad_fill = slice.end_angle_deg * M_PI / 180.0;
        int num_fill_lines = radius * 1000; // Dolgu yoðunluðu (daha fazla çizgi = daha yoðun)
        if (num_fill_lines <= 0) num_fill_lines = 1; // En az bir çizgi
        // Açýsal adým (radyan cinsinden)
        double angle_step_rad_fill = (fabs(end_rad_fill - start_rad_fill) < 1e-6 || num_fill_lines == 0) ? 0 : (end_rad_fill - start_rad_fill) / static_cast<double>(num_fill_lines);


        if (fabs(end_rad_fill - start_rad_fill) > 1e-6) { // Sadece anlamlý bir açý varsa doldur
            for (int j = 0; j <= num_fill_lines; ++j) {
                double current_fill_angle_rad = start_rad_fill + j * angle_step_rad_fill;
                int x_on_arc_for_fill = center_x + static_cast<int>(radius * cos(current_fill_angle_rad));
                int y_on_arc_for_fill = center_y + static_cast<int>(radius * sin(current_fill_angle_rad));
                Line(img, center_x, center_y, x_on_arc_for_fill, y_on_arc_for_fill, slice.color);
            }
        }

        // Dilim hatlarýný dolgudan sonra tekrar çizerek belirginleþtir
        TiltedEllipseArc(img, center_x, center_y, radius, radius, 0, /*ellipse_tilt_angle*/
            slice.color, static_cast<int>(slice.start_angle_deg), static_cast<int>(slice.end_angle_deg));
        // Yayýn baþlangýç ve bitiþ noktalarýný merkeze birleþtiren çizgiler
        double start_rad_lines = slice.start_angle_deg * M_PI / 180.0;
        double end_rad_lines = slice.end_angle_deg * M_PI / 180.0;
        int x_start_on_arc = center_x + static_cast<int>(radius * cos(start_rad_lines));
        int y_start_on_arc = center_y + static_cast<int>(radius * sin(start_rad_lines));
        int x_end_on_arc = center_x + static_cast<int>(radius * cos(end_rad_lines));
        int y_end_on_arc = center_y + static_cast<int>(radius * sin(end_rad_lines));
        Line(img, center_x, center_y, x_start_on_arc, y_start_on_arc, slice.color);
        Line(img, center_x, center_y, x_end_on_arc, y_end_on_arc, slice.color);
    }

    // Lejantý Çiz
    int legend_x_base = center_x + radius + legend_initial_x_offset;
    int legend_y_base = top_margin_for_title + legend_initial_y_offset;
    for (size_t i = 0; i < slices_param.size(); ++i) {
        const auto& slice = slices_param[i];
        int current_y_for_elements = legend_y_base + static_cast<int>(i * legend_item_spacing_y);

        // Lejantýn resim dýþýna taþmasýný engelle
        if (current_y_for_elements + legend_color_box_size > image_height - 5) break;

        // Renk kutucuðu
        FillRect(img, legend_x_base, current_y_for_elements, legend_color_box_size, legend_color_box_size, slice.color);
        // Etiket metni
        char legend_text[100];
        sprintf_s(legend_text, sizeof(legend_text), "%s (%.1f%%)", slice.label.c_str(), slice.percentage);
        Impress12x20(img, legend_x_base + legend_color_box_size + 5, current_y_for_elements, legend_text, textcolor);
    }
}

// --- GenerateAndDisplayPieChart_Main_GUI Implementasyonu ---
void GenerateAndDisplayPieChart_Main_GUI(const std::vector<std::pair<std::string, double>>& input_raw_data, const char* dynamic_title) {
    std::vector<PieSliceInfo> local_slices_info_list;
    double total_value = 0;

    if (!input_raw_data.empty()) {
        for (const auto& item : input_raw_data) {
            if (item.second > 0) total_value += item.second; // Sadece pozitif deðerleri topla
        }
    }

    if (total_value > 1e-9 && !input_raw_data.empty()) {
        double current_angle_deg = 0; // Baþlangýç açýsý (genellikle saat 3 yönü)
        std::vector<unsigned int> colors = { // Daha canlý bir renk paleti
            0xFFE53935, // Kýrmýzý
            0xFF1E88E5, // Mavi
            0xFF43A047, // Yeþil
            0xFFFFB300, // Amber/Sarý
            0xFF8E24AA, // Mor
            0xFF00ACC1, // Cyan
            0xFFFDD835, // Koyu Sarý
            0xFFD81B60, // Pembe
            0xFF546E7A  // Mavi Gri
        };
        int color_index = 0;

        for (size_t idx = 0; idx < input_raw_data.size(); ++idx) {
            const auto& item = input_raw_data[idx];
            if (item.second <= 0) continue; // Negatif veya sýfýr deðerleri atla

            PieSliceInfo slice;
            slice.label = item.first; // Türkçe karakter sorununu çözmek için ASCII kullanýn
            slice.value = item.second;
            slice.percentage = (item.second / total_value) * 100.0;
            slice.start_angle_deg = current_angle_deg;
            double slice_angle_span = (slice.percentage / 100.0) * 360.0;

            // Çok küçük yüzdeli dilimlerin bile en azýndan biraz görünür olmasý için minimum açý
            if (slice_angle_span < 0.5 && slice.percentage > 0.001) slice_angle_span = 0.5;
            // Eðer dilim çok küçükse ve açý sýfýrsa, son dilimdeysek ve boþluk kalmýþsa o boþluðu alsýn
            if (idx == input_raw_data.size() - 1 && (current_angle_deg + slice_angle_span < 359.5)) {
                slice_angle_span = 360.0 - current_angle_deg;
            }

            slice.end_angle_deg = current_angle_deg + slice_angle_span;


            slice.color = colors[color_index % colors.size()];
            color_index++;
            local_slices_info_list.push_back(slice);
            current_angle_deg = slice.end_angle_deg;
        }
        // Son dilimin tam 360'a gelmesini saðlamak (eðer toplam 360 deðilse)
        if (!local_slices_info_list.empty() && local_slices_info_list.back().end_angle_deg < 359.9) {
            local_slices_info_list.back().end_angle_deg = 360.0;
        }
    }

    // Grafik için genel parametreler
    int img_w = 950; // Çerçeve geniþliðiyle uyumlu
    int img_h = 500; // Çerçeve yüksekliðiyle uyumlu

    // Pasta grafiðin merkezini ve yarýçapýný hesapla
    int defined_top_margin_for_title = 30; // CreatePieChart içindekiyle uyumlu olmalý
    int approx_legend_width = 250; // Lejant için yaklaþýk geniþlik tahmini

    // Pastanýn çizilebileceði maksimum yarýçapý belirle
    // Yükseklikten: (toplam yükseklik - baþlýk marjý - alt marj) / 2
    int radius_from_height = (img_h - defined_top_margin_for_title - 20) / 2;
    // Geniþlikten: (toplam geniþlik - lejant geniþliði - sol/sað marjlar) / 2
    int radius_from_width = (img_w - approx_legend_width - 50) / 2; // 50px sol/sað toplam marj

    int radius_val = std::min(radius_from_height, radius_from_width);
    if (radius_val < 20) radius_val = 20; // Minimum yarýçap

    // Merkez X: Yarýçap + sol marj
    int center_x_val = radius_val + 30; // 30px sol marj
    // Merkez Y: Baþlýk marjý + yarýçap (veya dikeyde ortala)
    int center_y_val = defined_top_margin_for_title + radius_val + 10; // 10px baþlýk altý boþluk


    unsigned int bg_color_param = 0xFFF5F5F5;
    unsigned int text_color_param = 0xFF1A1A1A; // Biraz daha koyu metin

    CreatePieChart(pie_chart_image_global_obj, local_slices_info_list, dynamic_title, // Dinamik baþlýðý kullan
        img_w, img_h, center_x_val, center_y_val, radius_val,
        bg_color_param, text_color_param);

    DisplayImage(FRM_PieChart_Display_Handle, pie_chart_image_global_obj);
}

// --- Test Fonksiyonlarý ---
void TestPieChartWithData1() {
    std::vector<std::pair<std::string, double>> data1 = {
        {"Yazilim Gelistirme", 45.0},
        {"Donanim Alimlari", 25.0},
        {"Teknik Destek Giderleri", 15.0},
        {"Egitimler", 10.0},
        {"Danismanlik", 5.0}
    };
    GenerateAndDisplayPieChart_Main_GUI(data1, "Proje Harcamalari Dagilimi");
}

void TestPieChartWithData2() {
    std::vector<std::pair<std::string, double>> data2 = {
        {"Elma", 120.0}, {"Armut", 80.0}, {"Muz", 150.0},
        {"Cilek", 95.0}, {"Portakal", 110.0}, {"Kivi", 70.0},
        {"Uzum", 60.0}, {"Ananas", 40.0}
    };
    GenerateAndDisplayPieChart_Main_GUI(data2, "Meyve Stok Durumu (Kg)");
}

void TestPieChartWithData3() {
    std::vector<std::pair<std::string, double>> data3 = {
        {"Elektronik", 320.0}, {"Giyim", 210.0}, {"Market", 450.0},
        {"Mobilya", 180.0}, {"Kitap", 95.0}, {"Oyuncak", 130.0}
    };
    GenerateAndDisplayPieChart_Main_GUI(data3, "Kategori Bazli Satis Dagilimi");
}

void TestPieChartWithData4() {
    std::vector<std::pair<std::string, double>> data4 = {
        {"Kira", 2500.0}, {"Fatura", 850.0}, {"Yiyecek", 1250.0},
        {"Ulasim", 600.0}, {"Eglence", 400.0}, {"Diger", 300.0}
    };
    GenerateAndDisplayPieChart_Main_GUI(data4, "Aylik Gider Dagilimi (TL)");
}


// --- ICGUI Fonksiyonlarý ---
void ICGUI_Create() {
    ICG_MWSize(1000, 600); // Ana pencere boyutu
    ICG_MWTitle("Dinamik Pasta Grafik - I-See-Bytes");
}

void ICGUI_main() {
    // Panel boyutunu GenerateAndDisplay... içindeki img_w, img_h ile eþleþtir
    FRM_PieChart_Display_Handle = ICG_FramePanel(10, 10, 720, 500);

    ICG_Button(10, 520, 220, 25, "Proje Harcamalari", TestPieChartWithData1); // Buton metni güncellendi
    ICG_Button(240, 520, 220, 25, "Meyve Stok Durumu", TestPieChartWithData2); // Buton metni güncellendi
    ICG_Button(470, 520, 220, 25, "Kategori Bazlý Satýþ Daðýlýmý", TestPieChartWithData3); // Buton metni güncellendi
    ICG_Button(700, 520, 220, 25, "Aylýk Gider Daðýlýmý (TL)", TestPieChartWithData4); // Buton metni güncellendi

    TestPieChartWithData1(); // Baþlangýçta bir veri seti ve baþlýkla çizdir
}