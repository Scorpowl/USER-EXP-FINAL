// main.cpp
// icbytes.h, icb_media.h ve icb_gui.h proje dizininde veya include path'lerde olmalý.
// I-See-Bytes kütüphanesiyle linklenmeli (örn: ICBYTESx64Release.lib)

#include "icbytes.h"   // Temel ICBYTES sýnýfý için
#include "ic_media.h" // Impress12x20, Line, FillRect vb. için
#include "icb_gui.h"   // ICGUI fonksiyonlarý için

#include <algorithm>   // std::min ve std::max için
#include <cmath>       // fabs için
#include <cstdio>      // sprintf_s için
#include <iostream>    // Hata ayýklama mesajlarý için
#include <vector>      // Geçici olarak string uzunluklarý için

// Global GUI deðiþkenleri
int FRM1_Chart_Display;
ICBYTES chart_image_to_display;

// BarChart Fonksiyonu
int* BarChart(ICBYTES& img, ICBYTES& data, const char* chart_title, int image_total_height_param, // Bu artýk toplam resim yüksekliði
    int barwidth = 14, unsigned int backcolor = 0xFFFFFFFF, unsigned int barcolor = 0xFF00AA00,
    unsigned int axiscolor = 0xFF000000, unsigned int textcolor = 0xFF000000) {

    int num_data_points = data.X();
    bool is_row_vector = true;

    if (data.Y() > 1 && data.X() == 1) {
        num_data_points = data.Y();
        is_row_vector = false;
    }
    else if (data.X() == 0 || data.Y() == 0) {
        num_data_points = 0;
    }
    else if (data.Y() > 1 && data.X() > 1) {
        std::cout << "BarChart Uyarisi: Veri 2D matris. Ilk satir kullaniliyor." << std::endl;
    }

    // Temel kontroller
    int min_chart_height_for_labels = 80; // Baþlýk ve min 1 etiket için
    if (image_total_height_param < min_chart_height_for_labels) image_total_height_param = min_chart_height_for_labels;
    if (barwidth <= 0) barwidth = 14;

    if (num_data_points == 0) {
        std::cerr << "BarChart Hatasi: Veri bos veya taninan 1D vektor degil." << std::endl;
        CreateImage(img, barwidth > 0 ? barwidth : 20, image_total_height_param, ICB_UINT);
        img = backcolor;
        Impress12x20(img, 2, image_total_height_param / 2 - 10, "Veri Yok", textcolor);
        return nullptr;
    }

    // Metin ve kenar boþluklarý için alan ayýr
    int top_margin_for_title = 25;      // Baþlýk için üst boþluk (Impress12x20 font yüksekliði ~20px)
    int bottom_margin = 5;              // Alt kenar boþluðu
    int left_margin_for_y_labels = 85;  // Y ekseni etiketleri ve eksen için sol boþluk (etiket uzunluðuna göre ayarlanabilir)
    int right_margin = 15;              // Sað kenar boþluðu

    // Asýl çizim alaný yüksekliði
    int chart_area_height = image_total_height_param - top_margin_for_title - bottom_margin;
    if (chart_area_height < 30) { // Çok küçük kalýrsa
        chart_area_height = 30;
        std::cout << "BarChart Uyarisi: Grafik cizim alani cok kucuk kaldi." << std::endl;
    }


    int chart_area_width = num_data_points * barwidth;
    int image_total_width = left_margin_for_y_labels + chart_area_width + right_margin;

    CreateImage(img, image_total_width, image_total_height_param, ICB_UINT);
    img = backcolor;

    // Grafik Baþlýðýný Çiz
    if (chart_title && strlen(chart_title) > 0) {
        int title_len_px = strlen(chart_title) * 12; // Yaklaþýk piksel uzunluðu (12px/char varsayýmý)
        int title_x = left_margin_for_y_labels + (chart_area_width - title_len_px) / 2;
        if (title_x < 5) title_x = 5;
        Impress12x20(img, title_x, 3, chart_title, textcolor); // Y pozisyonu biraz daha yukarýda
    }

    // 1. Verideki min ve max deðerleri bul
    double min_val = is_row_vector ? data.D(1, 1) : data.D(1, 1);
    double max_val = min_val;
    for (int i = 1; i <= num_data_points; ++i) {
        double current_val = is_row_vector ? data.D(i, 1) : data.D(1, i);
        if (current_val < min_val) min_val = current_val;
        if (current_val > max_val) max_val = current_val;
    }
    // Eðer tüm deðerler ayný ise, 0'ý da kapsayacak þekilde aralýðý geniþlet (görselleþtirme için)
    if (min_val == max_val) {
        if (min_val > 0) min_val = 0;
        else if (max_val < 0) max_val = 0;
        else { // Hepsi 0 ise
            max_val = 1; // göstermek için küçük bir aralýk
            min_val = -1;
        }
    }
    // 0 her zaman aralýkta olsun (negatif ve pozitif barlarýn daha iyi görünmesi için)
    if (max_val < 0) max_val = 0;
    if (min_val > 0) min_val = 0;


    // 5. X ekseninin Y piksel pozisyonunu ve ölçek faktörünü belirle (sadece chart_area_height içinde)
    double data_visual_range = max_val - min_val; // Görselleþtirilecek aralýk
    double scale_y;

    if (fabs(data_visual_range) < 1e-9) { // Bu durum artýk olmamalý (yukarýdaki min_val/max_val ayarý sayesinde)
        scale_y = 1.0; // Ama yine de bir fallback
    }
    else {
        scale_y = (double)chart_area_height / data_visual_range;
    }

    // 0 çizgisinin, chart_area'nýn tepesinden olan piksel uzaklýðý
    int y_zeroline_in_chart_area_px = (int)((max_val - 0.0) * scale_y);
    // Sýnýrlar içinde tut (chart_area_height için)
    if (y_zeroline_in_chart_area_px < 0) y_zeroline_in_chart_area_px = 0;
    if (y_zeroline_in_chart_area_px >= chart_area_height) y_zeroline_in_chart_area_px = chart_area_height - 1;

    // Gerçek resim üzerindeki Y pozisyonu (üst marjý ekleyerek)
    int y_zeroline_image_px = y_zeroline_in_chart_area_px + top_margin_for_title;


    // Y Ekseni Etiketlerini Çiz
    int num_y_axis_labels = 4; // Gösterilecek etiket sayýsý (0 çizgisi hariç)
    if (fabs(data_visual_range) > 1e-9) {
        for (int k = 0; k <= num_y_axis_labels; ++k) {
            double val_for_label = min_val + (data_visual_range / num_y_axis_labels) * k;
            // Etiketin, chart_area'nýn tepesinden olan piksel uzaklýðý
            int y_label_in_chart_area_px = (int)((max_val - val_for_label) * scale_y);
            // Resim üzerindeki Y pozisyonu
            int y_label_image_px = y_label_in_chart_area_px + top_margin_for_title;

            // Etiketin dikeyde ortalanmasý için metin yüksekliðinin yarýsý (~10px)
            int text_y_pos = y_label_image_px - 10;
            if (text_y_pos < 0) text_y_pos = 0; // Resim dýþýna taþmasýn
            if (text_y_pos > image_total_height_param - 20) text_y_pos = image_total_height_param - 20; // Alt sýnýrdan taþmasýn

            char label_text[32];
            sprintf_s(label_text, sizeof(label_text), "%.1f", val_for_label);
            int label_len_px = strlen(label_text) * 12; // Yaklaþýk metin geniþliði
            int text_x_pos = left_margin_for_y_labels - label_len_px - 55; // Eksenin soluna, saða dayalý
            if (text_x_pos < 2) text_x_pos = 2;

            Impress12x20(img, text_x_pos, text_y_pos, label_text, textcolor);
        }
    }

    // 6. X eksenini çiz (Y ekseni etiketleri çizildikten sonra, üzerini kapatmasýnlar)
    unsigned int belirgin_axis_color = 0xFF303030; // Daha koyu bir gri veya siyah (axiscolor parametresi yerine)
    // Ýstersen X eksenini 2 piksel kalýnlýðýnda çiz (üst üste iki çizgi)
    Line(img, left_margin_for_y_labels, y_zeroline_image_px, image_total_width - right_margin - 1, y_zeroline_image_px, belirgin_axis_color);
    if (y_zeroline_image_px + 1 < image_total_height_param) { // Sýnýr kontrolü
        Line(img, left_margin_for_y_labels, y_zeroline_image_px + 1, image_total_width - right_margin - 1, y_zeroline_image_px + 1, belirgin_axis_color);
    }
    Line(img, left_margin_for_y_labels, top_margin_for_title, left_margin_for_y_labels, top_margin_for_title + chart_area_height - 1, belirgin_axis_color);


    // 7. Çubuklarý çiz
    int bar_spacing = 2; // Çubuklar arasýndaki boþluk (piksel) - barwidth'ten büyük olmamalý!
    if (barwidth <= bar_spacing * 2) bar_spacing = 0; // Çok dar çubuklarda boþluk olmaz

    int actual_bar_width = barwidth - bar_spacing; // Her çubuðun gerçek çizim geniþliði
    if (actual_bar_width < 1) actual_bar_width = 1; // Minimum 1 piksel geniþlik

    for (int i = 0; i < num_data_points; ++i) {
        double current_data_val = is_row_vector ? data.D(i + 1, 1) : data.D(1, i + 1);

        int bar_pixel_height_abs = (int)(fabs(current_data_val) * scale_y);
        if (bar_pixel_height_abs == 0 && fabs(current_data_val) > 1e-9) bar_pixel_height_abs = 1;

        // Çubuðun X baþlangýcý, sol marj + (çubuk no * toplam çubuk alaný) + yarým boþluk
        int bar_x_start_image_px = left_margin_for_y_labels + (i * barwidth) + (bar_spacing / 2);
        if (bar_spacing == 0) bar_x_start_image_px = left_margin_for_y_labels + (i * barwidth);


        int bar_y_start_image_px;

        if (current_data_val >= 0) {
            bar_y_start_image_px = y_zeroline_image_px - bar_pixel_height_abs;
            if (bar_y_start_image_px < top_margin_for_title) {
                bar_pixel_height_abs -= (top_margin_for_title - bar_y_start_image_px);
                bar_y_start_image_px = top_margin_for_title;
            }
            if (bar_pixel_height_abs < 0) bar_pixel_height_abs = 0;
            FillRect(img, bar_x_start_image_px, bar_y_start_image_px, actual_bar_width, bar_pixel_height_abs, barcolor);
        }
        else {
            bar_y_start_image_px = y_zeroline_image_px + 1; // X eksen çizgisinin hemen altýndan baþla
            // Negatif çubuklar için X eksenini kalýnlaþtýrdýysak +2'den baþlatabiliriz
           // if (y_zeroline_image_px + 1 < image_total_height_param) bar_y_start_image_px = y_zeroline_image_px + 2;


            if (bar_y_start_image_px + bar_pixel_height_abs >= top_margin_for_title + chart_area_height) {
                bar_pixel_height_abs = (top_margin_for_title + chart_area_height) - bar_y_start_image_px;
            }
            if (bar_pixel_height_abs < 0) bar_pixel_height_abs = 0;
            FillRect(img, bar_x_start_image_px, bar_y_start_image_px, actual_bar_width, bar_pixel_height_abs, barcolor);
        }
    }
}

// --- GUI Uygulamasý ---
void GenerateAndDisplayChart_Main_GUI() {
    ICBYTES test_data_for_chart;
    double actual_data_array[] = { 120.5, 150.2, 90.0, -30.5, 0.0, 75.8, 180.0, -50.9, 110.0, 20.0, 55.0, -80.0 };
    int num_elements_in_array = sizeof(actual_data_array) / sizeof(actual_data_array[0]);
    CreateMatrix(test_data_for_chart, num_elements_in_array, 1, ICB_DOUBLE);
    for (int k = 0; k < num_elements_in_array; ++k) {
        test_data_for_chart.D(k + 1, 1) = actual_data_array[k];
    }

    const char* title_for_chart = "Aylik Satis Performansi (Bin TL)"; // ASCII Karakterler
    int total_image_height = 450;
    int width_per_bar = 30;
    unsigned int background_color_chart = 0xFFFAFAFA;
    unsigned int color_for_bars = 0xFF303F9F;   // Koyu Ýndigo
    unsigned int color_for_axis = 0xFF757575;   // Orta Gri eksen
    unsigned int color_for_text = 0xFF000000;

    BarChart(chart_image_to_display, test_data_for_chart, title_for_chart, total_image_height,
        width_per_bar, background_color_chart, color_for_bars, color_for_axis, color_for_text);

    // Oluþturulan resmin boyutlarýna göre Frame panelini ayarla (eðer dinamikse)
    // Þimdilik sabit ICGUI_main içinde ayarlýyoruz.
    DisplayImage(FRM1_Chart_Display, chart_image_to_display);
}

void ICGUI_Create() {
    ICG_MWSize(950, 600); // Ana pencereyi biraz büyütelim
    ICG_MWTitle("Gelistirilmis Bar Grafik - I-See-Bytes");
}

void ICGUI_main() {
    // Frame panelinin boyutunu BarChart'ýn üreteceði resim boyutuna göre ayarlamak ideal olurdu.
    // Þimdilik BarChart içindeki image_total_width ve image_total_height_param'a yakýn deðerler verelim.
    // Test verisi için: 12 çubuk * 30px + sol(55) + sað(15) = 360 + 70 = 430px geniþlik. Yükseklik 450px.
    FRM1_Chart_Display = ICG_FramePanel(10, 10, 500, 500); // Boyutlarý grafiðe göre ayarla

    ICG_Button(10, 520, 250, 25, "Grafigi Yeniden Olustur", GenerateAndDisplayChart_Main_GUI);

    GenerateAndDisplayChart_Main_GUI();
}