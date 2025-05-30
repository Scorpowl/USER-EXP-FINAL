// main.cpp
#include "icbytes.h"
#include "ic_media.h"
#include "icb_gui.h"

#include <vector>
#include <string>
#include <numeric>   // std::accumulate için
#include <cmath>       // M_PI, cos, sin için (gerçi M_PI Windows'ta doðrudan tanýmlý olmayabilir)
#include <cstdio>      // sprintf_s için
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Global GUI deðiþkenleri
int FRM_PieChart_Display;
ICBYTES pie_chart_image_global;

// Yardýmcý yapý, her dilim için bilgi tutar
struct PieSliceInfo {
    std::string label;
    double value;
    double percentage;
    double start_angle_deg;
    double end_angle_deg;
    unsigned int color;
};

// Pasta Grafik Fonksiyonu
void CreatePieChart(ICBYTES& img, const std::vector<PieSliceInfo>& slices,
    const char* chart_title, int image_width, int image_height,
    int center_x, int center_y, int radius,
    unsigned int backcolor = 0xFFFFFFFF, unsigned int textcolor = 0xFF000000) {

    // Marjlar ve diðer sabitler
    int top_margin_for_title = 30;      // Baþlýk için üst boþluk
    int legend_label_offset_x = 25;   // Lejantta renk kutucuðu ile metin arasý boþluk
    int legend_color_box_size = 15;   // Lejanttaki renk kutucuðunun boyutu
    int legend_item_spacing_y = 25;   // Lejanttaki satýrlar arasý dikey boþluk
    int legend_initial_x_offset = 30; // Pastanýn saðýndan lejantýn ne kadar uzakta baþlayacaðý
    int legend_initial_y_offset = 20; // Baþlýðýn altýndan lejantýn ne kadar aþaðýda baþlayacaðý


    CreateImage(img, image_width, image_height, ICB_UINT);
    img = backcolor;

    // Baþlýk
    if (chart_title && strlen(chart_title) > 0) {
        int title_len_px = strlen(chart_title) * 12; // Yaklaþýk piksel uzunluðu (12px/char varsayýmý)
        int title_x_pos = (image_width - title_len_px) / 2; // Resmi ortala
        if (title_x_pos < 5) title_x_pos = 5; // Kenara çok yapýþmasýn
        // Impress12x20 font yüksekliði ~20px. Marjýn ortasýna yerleþtirmek için:
        Impress12x20(img, title_x_pos, (top_margin_for_title - 20) / 2, chart_title, textcolor);
    }

    if (slices.empty()) {
        Impress12x20(img, 10, top_margin_for_title + 10, "Pasta grafik icin veri yok.", textcolor);
        return;
    }

    // Dilimleri Çiz
    for (const auto& slice : slices) {
        TiltedEllipseArc(img, center_x, center_y, radius, radius, 0,
            slice.color, static_cast<int>(slice.start_angle_deg), static_cast<int>(slice.end_angle_deg));

        double start_rad = slice.start_angle_deg * M_PI / 180.0;
        double end_rad = slice.end_angle_deg * M_PI / 180.0;

        // Yayýn baþlangýç ve bitiþ noktalarýný hesapla (Y ekseni aþaðý doðru artar)
        // GDI+'da veya benzeri sistemlerde Y genellikle yukarý doðru artar, bu yüzden sinüs negatif olur.
        // Ancak I-See-Bytes'ýn Line fonksiyonunun nasýl çalýþtýðýna baðlý.
        // Ekran koordinatlarý (sol üst 0,0, Y aþaðý) için sinüs pozitif kalmalý.
        int x_start_on_arc = center_x + static_cast<int>(radius * cos(start_rad));
        int y_start_on_arc = center_y + static_cast<int>(radius * sin(start_rad));

        int x_end_on_arc = center_x + static_cast<int>(radius * cos(end_rad));
        int y_end_on_arc = center_y + static_cast<int>(radius * sin(end_rad));

        Line(img, center_x, center_y, x_start_on_arc, y_start_on_arc, slice.color);
        Line(img, center_x, center_y, x_end_on_arc, y_end_on_arc, slice.color);
		// Yayýn baþlangýç ve bitiþ noktalarýný birleþtir

        int num_fill_lines = radius * 3000; // Yoðunluða göre ayarla (örn: radius kadar veya yarýsý)
        for (int j = 0; j <= num_fill_lines; ++j) {
            double angle_rad = (slice.start_angle_deg + (slice.end_angle_deg - slice.start_angle_deg) * ((double)j / num_fill_lines)) * M_PI / 180.0;
            int x_on_arc = center_x + static_cast<int>(radius * cos(angle_rad));
            int y_on_arc = center_y + static_cast<int>(radius * sin(angle_rad));
            Line(img, center_x, center_y, x_on_arc, y_on_arc, slice.color);
        }
        // Ardýndan dýþ yayý ve merkezden kenarlara çizgileri tekrar çizerek hatlarý belirginleþtir.
        TiltedEllipseArc(img, center_x, center_y, radius, radius, 0, slice.color, static_cast<int>(slice.start_angle_deg), static_cast<int>(slice.end_angle_deg));
        Line(img, center_x, center_y, x_start_on_arc, y_start_on_arc, slice.color); // x_start_on_arc vb. daha önce hesaplanmýþtý
        Line(img, center_x, center_y, x_end_on_arc, y_end_on_arc, slice.color);
    
    
    }


    // Lejant / Etiketler
    int legend_x_start = center_x + radius + legend_initial_x_offset;
    int legend_y_start = top_margin_for_title + legend_initial_y_offset;

    for (size_t i = 0; i < slices.size(); ++i) {
        const auto& slice = slices[i];
        // Lejantýn Y pozisyonu, metnin dikeyde ortalanmasý için metin yüksekliðinin yarýsý (~10px) düþülerek
        int current_y_for_text = legend_y_start + i * legend_item_spacing_y;
        int current_y_for_box = current_y_for_text; // Kutu ve metin ayný hizada baþlasýn

        if (current_y_for_box + legend_color_box_size > image_height - 5) break; // Lejant resim dýþýna taþýyorsa çizme

        FillRect(img, legend_x_start, current_y_for_box, legend_color_box_size, legend_color_box_size, slice.color);

        char legend_text[100];
        sprintf_s(legend_text, sizeof(legend_text), "%s (%.1f%%)", slice.label.c_str(), slice.percentage);
        Impress12x20(img, legend_x_start + legend_color_box_size + 5, current_y_for_text, legend_text, textcolor); // Renk kutusundan 5px saða
    }
}


// --- GUI Uygulamasý ---
void GenerateAndDisplayPieChart_Main_GUI(const std::vector<std::pair<std::string, double>>& input_raw_data) {
    std::vector<PieSliceInfo> local_slices_info; // Fonksiyon içinde oluþturulsun

    double total_value = 0;
    for (const auto& item : input_raw_data) { // Gelen parametreyi kullan
        total_value += item.second;
    }

    if (total_value > 1e-9) {
        double current_angle_deg = 0;
        std::vector<unsigned int> colors = {
            0xFFE91E63, 0xFF9C27B0, 0xFF2196F3, 0xFF4CAF50, 0xFFFFC107, 0xFFFF5722,
            0xFF795548, 0xFF607D8B, 0xFF00BCD4, 0xFF8BC34A // Daha fazla renk ekle
        };
        int color_index = 0;

        for (const auto& item : input_raw_data) { // Gelen parametreyi kullan
            PieSliceInfo slice;
            slice.label = item.first; // Etiket için ASCII karakterler kullanmaya devam et
            slice.value = item.second;
            slice.percentage = (item.second / total_value) * 100.0;
            slice.start_angle_deg = current_angle_deg;
            slice.end_angle_deg = current_angle_deg + (slice.percentage / 100.0) * 360.0;
            // Küçük yüzdeli dilimlerin yayýnýn en az 1 derece olmasý için (görsel olarak)
            if (slice.end_angle_deg - slice.start_angle_deg < 1.0 && slice.percentage > 0) {
                slice.end_angle_deg = slice.start_angle_deg + 1.0;
            }
            slice.color = colors[color_index % colors.size()];
            color_index++;
            local_slices_info.push_back(slice);
            current_angle_deg = slice.end_angle_deg;
        }
    }
    current_pie_slices_info = local_slices_info; // Global'e kopyala

    const char* pie_chart_title_text = "Dinamik Veri ile Pasta Grafik"; // Baþlýðý deðiþtirebilirsin
    int img_w = 700;
    int img_h = 450;
    // Bu global deðiþkenlerin CreatePieChart tarafýndan kullanýlacaðýný unutma
    pie_chart_center_x_global = 220; // Lejant için saðda daha fazla yer býrakmak adýna biraz sola
    pie_chart_center_y_global = img_h / 2 + 10;
    pie_chart_radius_global = (img_h / 2) - 40; // Yüksekliðe göre yarýçapý ayarla (baþlýk ve alt boþluk için)
    // Yarýçapý biraz küçülttüm ki lejant daha rahat sýðsýn

    unsigned int bg_color_param = 0xFFFAFAFA;
    unsigned int bar_col_param_dummy = 0; // Pasta grafikte bar rengi yok, dilim renkleri var
    unsigned int axis_color_param_dummy = 0; // Pasta grafikte eksen rengi yok
    unsigned int text_color_param = 0xFF000000;


    CreatePieChart(pie_chart_image_global, current_pie_slices_info, pie_chart_title_text,
        img_w, img_h, pie_chart_center_x_global, pie_chart_center_y_global, pie_chart_radius_global,
        bg_color_param, text_color_param); // barcolor ve axiscolor dummy

    DisplayImage(FRM_PieChart_Display, pie_chart_image_global);
}


    // Grafik parametreleri
    const char* pie_chart_title_text = "Departman Harcama Dagilimi"; // ASCII
    int img_w = 700; // Resim geniþliði
    int img_h = 450; // Resim yüksekliði
    int pie_center_x = 200; // Pasta merkez X (sol marjdan sonra)
    int pie_center_y = img_h / 2 + 10; // Pasta merkez Y (baþlýktan sonra ortala)
    int pie_radius = 150;

    CreatePieChart(pie_chart_image_global, slices_info, pie_chart_title_text,
        img_w, img_h, pie_center_x, pie_center_y, pie_radius,
        0xFFFAFAFA, 0xFF000000);

    DisplayImage(FRM_PieChart_Display, pie_chart_image_global);
}

void ICGUI_Create() {
    ICG_MWSize(750, 550);
    ICG_MWTitle("Pasta Grafik Uygulamasi - I-See-Bytes");
}

void ICGUI_main() {
    FRM_PieChart_Display = ICG_FramePanel(10, 10, 720, 500);
    ICG_Button(10, 520, 250, 25, "Pastayi Yeniden Ciz", GenerateAndDisplayPieChart_Main_GUI);
    GenerateAndDisplayPieChart_Main_GUI();
}