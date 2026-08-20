#include "common_code.hpp"
#include <opencv2/imgproc/imgproc.hpp>

void 
fsiv_find_min_max_loc_1(cv::Mat const& input,
    std::vector<cv::uint8_t>& min_v, std::vector<cv::uint8_t>& max_v,
    std::vector<cv::Point>& min_loc, std::vector<cv::Point>& max_loc)
{
    CV_Assert(input.depth() == CV_8U);

    // Separamos los canales de la imagen de entrada
    std::vector<cv::Mat> canales;
    cv::split(input, canales);

    size_t num_channels = canales.size();
    
    // Redimensionamos los vectores de salida para que coincidan con los canales
    min_v.resize(num_channels);
    max_v.resize(num_channels);
    min_loc.resize(num_channels);
    max_loc.resize(num_channels);

    // Recorrido por filas y columnas (Método intuitivo basado en compute_stats1)
    for (size_t c = 0; c < num_channels; ++c)
    {
        const cv::Mat& chan = canales[c];
        
        // Inicializamos con el primer píxel del canal
        uint8_t current_min = chan.at<uint8_t>(0, 0);
        uint8_t current_max = chan.at<uint8_t>(0, 0);
        cv::Point current_min_loc(0, 0);
        cv::Point current_max_loc(0, 0);

        for (int r = 0; r < chan.rows; ++r)
        {
            for (int col = 0; col < chan.cols; ++col)
            {
                uint8_t val = chan.at<uint8_t>(r, col);

                // Para encontrar el "primer" mínimo/máximo mantenemos '<' o '>'
                if (val < current_min)
                {
                    current_min = val;
                    current_min_loc = cv::Point(col, r); // OpenCV usa Point(x, y) -> (columna, fila)
                }
                if (val > current_max)
                {
                    current_max = val;
                    current_max_loc = cv::Point(col, r);
                }
            }
        }

        // Guardamos los resultados del canal en los vectores
        min_v[c] = current_min;
        max_v[c] = current_max;
        min_loc[c] = current_min_loc;
        max_loc[c] = current_max_loc;
    }

    CV_Assert(input.channels() == min_v.size());
    CV_Assert(input.channels() == max_v.size());
    CV_Assert(input.channels() == min_loc.size());
    CV_Assert(input.channels() == max_loc.size());
}

void 
fsiv_find_min_max_loc_2(cv::Mat const& input,
    std::vector<double>& min_v, std::vector<double>& max_v,
    std::vector<cv::Point>& min_loc, std::vector<cv::Point>& max_loc)
{
    // Separamos los canales de la imagen de entrada
    std::vector<cv::Mat> canales;
    cv::split(input, canales);

    size_t num_channels = canales.size();

    // Redimensionamos los vectores de salida para que coincidan con los canales
    min_v.resize(num_channels);
    max_v.resize(num_channels);
    min_loc.resize(num_channels);
    max_loc.resize(num_channels);

    // Uso de la función nativa y optimizada de OpenCV cv::minMaxLoc (Basado en compute_stats4)
    for (size_t c = 0; c < num_channels; ++c)
    {
        double min_val, max_val;
        cv::Point min_pt, max_pt;

        cv::minMaxLoc(canales[c], &min_val, &max_val, &min_pt, &max_pt);

        min_v[c] = min_val;
        max_v[c] = max_val;
        min_loc[c] = min_pt;
        max_loc[c] = max_pt;
    }

    CV_Assert(input.channels() == min_v.size());
    CV_Assert(input.channels() == max_v.size());
    CV_Assert(input.channels() == min_loc.size());
    CV_Assert(input.channels() == max_loc.size());
}



for(size_t c=0;c<num_chaneles; ++c)
double max_v, min_v;
cv::Point max_l, min_l;
cv::minMaxLoc(canales[c], &min_v, &max_v,& max_l,& min_l);

max_v[c] =max_val;
min_v[c] =min_val;
max_l[c] =max_loc;
min_l[c] =min_loc;












void 
fsiv_min_max_loc_2(cv::Mat &const input,
                   std::vector<double> &max_val,
                   std::vector<double> &min_val, 
                   std::vector<cv::Point> &max_loc,
                   std::vector<cv::Point> &min_loc)
{

   std::vector<cv::Mat> canales;
   cv::split(input,canales);

   size_t num_canales=canales.size();
   min_val.resize(num_canales);
   max_val.resize(num_canales);
   max_loc.resize(num_canales);
   min_loc.resize(num_canales);

   for(size_t c=0; c<mum_canales; ++c)
   {
      double min_v, max_v;
      cv::Point min_l,max_l;
      cv::minMaxLoc(canales[c],&max_v, &min_v,&max_l,&min_l);

      min_val[c]=min_v;
      max_val[c]=max_v;
      min_loc[c]=min_l;
      max_loc[c]=max_l;
    }
    cv_assert(input.channeles()==min_val.size());
    cv_assert(input.channeles()==max_val.size());
    cv_assert(input.channeles()==min_loc.size());
    cv_assert(input.channeles()==max_loc.size());


}

















void
fsiv_min_max_loc(cv::Mat &const input,
                 std::<vector>double &max_val,
                 std::<vector>double &min_val,
                 std::<vector>cv::Point &max_loc,
                 std::<vector>cv::Poitt &min_loc)
{
//separamos los canales 
std::<vector>cv::Mat canales;
cv::split(input,canales);
size_t num_canales=canales.size();

max_val=num_canales.resize();
min_val=num_canales.resize();
min_loc=num_canales.resize();
max_loc=num-canales.resize();

for(size_t c=0; c<num_canales; ++c){
    double max_v,min_v;
    cv::Point max_l, min_loc;

    cv::minMaxLoc(canales[c],&min_v, &max_v,&min_l,&max_l);
    max_val[c]=max_v;
    min_val[c]=min_v;
    max_loc[c]=max_l;
    min_loc[c]=min_l;
}
CV_assret(input.channels()==max_val.size());
CV_assert(input.channels()==min_val.size());
CV_assert(input.channls()==max_loc.size());
CV_assert(input.channels()==mic_loc.size());



}
























    
