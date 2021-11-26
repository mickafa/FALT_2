#include <bits/stdc++.h>
using namespace std ;
#define Me (5.9726e24)      /// масса Земли    /// индекс е = Earth
#define Re (6378100.0)      /// экваториальный радиус
#define Mm (7.3477e22)      /// масса Луны      /// индекс m = Moon
#define Rm (1737100.0)      /// радиус Луны
#define Lem (384467000.0)   /// расстояние между центрами Земли и Луны

#define G (6.6743015e-11)   /// гравитационная постоянная
#define DT (5.0)            /// шаг расчета по времени, в секундах -- РАБОТАЕТ ТОЛЬКО ПРИ 5 !!!
#define TICKER (10)          /// через сколько витков выдаём репорты на экран

#define TICKER_ORBIT_1 (10000) /// периодичность записи точек орбиты на этапе разгона под парусом  /// БЫЛО 10,000
#define TICKER_ORBIT_2 (220) /// периодичность записи точек орбиты после импульса двигателем

#define M   (10.)           /// масса солнечного парусника
#define S   (100.)          /// площадь солнечного паруса
#define P   (9.0e-6)        /// давление солнечного ветра
#define A   ((S*P)/M)       /// ускорение парусника от солнечного ветра, действует по оси Х

#define IMPULSE (14270629)  /// на этом тике дадим тормозной импульс (подобран опытным путем)

int ticker_orbit ;          /// частота записи точек орбиты

unsigned long long i;       /// количество тиков времени длительностью DT с начала полёта
int n = 0 ;                 /// количество витков парусника - оборотов вектора скорости

FILE *fo;                   /// файл для записи траектории полёта

double  ae, am, axe, aye, axm, aym, /// текущие ускорения, и по осям от Земли и Луны
        phie, phim, re, rm, /// текущее угловое положение Земли и Луны и их радиусы орбит
        xe, ye, xm, ym,     /// координаты Земли и Луны
        vx, vy, v,          /// текущие скорости парусника по осям и суммарная
        x, y,               /// текущие координаты парусника по осям
        t,                  /// время с начала полёта
        r, phi,             /// радиус и угол phi парусника - в полярных координатах
        r0e, r0m,           /// расстояние от парусника до центров Земли и Луны (для расчета силы тяжести)
        alpha,              /// угол между вектором скорости и осью ОХ
        xi, xi_new ;        /// (r,v) - скалярное произведение радиус-вектора и вектора скорости
double  omegaem;            /// угловая скорость вращения пары Земля-Луна вокруг барицентра
double  r0m_min = Rm * 10.; /// контроль на столкновение с Луной

double a ( double radius, double massa ){
/// ускорение парусника от силы притяжения к Земле или Луне         a = F / M(парусника)
    return  -( G * massa / (radius*radius ) ) ; /// знак минус потому, что ускорение влево и вниз
}

int init ( ) {                      /// инициализация задачи
/// вычисляем параметры системы Земля-Луна
    double omega2 ;
    double u ;                      /// переносная скорость - для смены СО с центра Земли на барицентр
    omega2 = G * ( Me + Mm ) /  ( Lem * Lem * Lem ) ;
    omegaem = sqrt ( omega2 );      /// угловая скорость Земли и Луны относительно барицентра

                                            /// переход в СО барицентра
    re = G * Mm  / ( omega2 * Lem * Lem ) ; /// расстояние от барицентра до центра Земли
    rm = G * Me  / ( omega2 * Lem * Lem ) ; /// расстояние от барицентра до центра Луны

/// посчитанный период обращения Луны, расстояния от барицентра до ц.Земли и ц.Луны
  printf ( "T=%.3lf days, re=%.3lf km, rm=%.3lf km\n", (2.*M_PI/omegaem )/(60.*60.*24.), re/1.e3, rm/1.e3 );

/// переходим в СО барицентра и устанавливаем начальные координаты
/// парусника, Земли, Луны и скорость парусника
const double x0 = 42174033.,    /// х-координата и скорость ракеты в момент прохождения
             v0 = 3074. ;       /// нулевой точки на ГСО (из первой задачи)

    x = x0 - re ;               /// новые координаты парусника
    y = 0.001 ;                 /// 1 миллиметр - чтобы не сработал флаг нулевой точки
    r = sqrt ( x*x + y*y ) ;    /// и в полярных координатах
    phi = atan2 ( y, x ) ;

    u = omegaem * re ;          /// линейная скорость ц.Земли относительно барицентра - это ПЕРЕНОСНАЯ
    vy = v = v0 - u ;           /// новая начальная скорость парусника
    vx = 0. ;

    phim = 1.02 * M_PI / 180. ; /// стартовая фаза Луны: запускаем Луну с угла 1,02 градуса (подобрано)
    xm = rm * cos ( phim ) ;    /// начальные прямоугольные координаты Луны
    ym = rm * sin ( phim ) ;

    phie = phim + M_PI ;        /// Земля всегда находится в противоположной фазе от Луны
    xe = re * cos ( phie ) ;    /// на старте Земля будет на угле phie = 180 градусов.
    ye = re * sin ( phie ) ;    /// Начальные прямоугольные координаты Земли

    t = 0.0 ;                   ///  часы - в ноль
    n = 0 ;                     ///  счётчик витков - в ноль
    return 0 ;
}  ///  инициализация закончена


int report ( ) {
/// В репорт даём: номер витка; апогей в тыс км.; угол апогея; скорость; альфа (между скоростью и ОХ);
/// полярная коорд. phi Луны; расстояние до центра Луны; время с начала полёта (дней)
    static char format[] = "%3d\t%7.2lf\t\t%7.1lf\t\t%6.1lf\t%7.1lf\t%7.1lf\t\t%9.1lf\t%.2lf\n" ;
/// Репорты выдаём только на экран. В файл не пишем, нет необходимости
    printf ( format , n, r/1e6, phi * 180./M_PI, v, alpha * 180. / M_PI, phim * 180. / M_PI,
                      r0m/1e6, t/(24.*60.*60.) ) ;
    return 0 ;
}

int orbit ( ) {
/// запись точек траектории парусника и Луны, только в файл. На экран не выдаём
    static char format[] = "%7.3lf;%7.3lf;%7.3lf;%7.3lf\n" ;
    fprintf ( fo, format, x/1.e6, y/1.e6, xm/1.e6, ym/1.e6 ) ;
    return 0 ;
}

int main ( ) {
int ticker = TICKER ;                       /// периодичность записи репортов, в витках
    fo = fopen ("orbit.csv" , "w" ) ;       /// сюда пишем траекторию
    assert ( fo ) ;

    init ( );                               /// инициализируем задачу
    printf (     "n\tApogee\t\tphi_apo\t\tv_apo\t alpha\tphi_Moon\tr0_Moon\t\tdays\n");

    ticker_orbit = TICKER_ORBIT_1 ;         /// частота записи точек траектории - низкая

    for ( i = 0L ; ; i++ ) {                /// полетели !
        if ( i % ticker_orbit == 0 ) orbit ( ) ;       /// пишем точку траектории
        double dx, dy, phiphi,  cos_phi, sin_phi;      /// временные переменные

        r = sqrt ( x*x + y*y );             /// полярные координаты парусника
        phi = atan2 ( y, x );

        dx = x - xe ;                       /// расстояние от парусника до Земли по осям
        dy = y - ye ;
        r0e = sqrt ( dx*dx + dy*dy ) ;      /// расстояние от парусника до Земли по прямой
        phiphi = atan2 ( dy , dx ) ;        /// угол между осью Х и направлением от парусника на Землю
        ae = a ( r0e , Me ) ;               /// ускорение парусника от Земли
        axe = ae * cos ( phiphi ) ;         /// проекции ускорения на оси
        aye = ae * sin ( phiphi ) ;

        dx = x - xm ;                       /// расстояние от парусника до Луны по осям
        dy = y - ym ;
        r0m = sqrt ( dx*dx + dy*dy ) ;      /// расстояние от парусника до Луны по прямой
        phiphi = atan2 ( dy , dx ) ;        /// угол между осью Х и направлением от парусника на Луну
        am = a ( r0m , Mm ) ;               /// ускорение парусника от Луны
        axm = am * cos ( phiphi ) ;         /// проекции ускорения на оси
        aym = am * sin ( phiphi ) ;

        alpha = atan2 ( vy, vx ) ;          /// угол между вектором скорости и осью ОХ

/// программа управления теперь простая: парус открыт не в нижнем секторе орбиты, а когда вектор
/// скорости имеет направление с осью Х от -90 до +85 градусов. Пробами нашёл, что
/// такой интервал даёт самый быстрый разгон
        if ( i <= IMPULSE )  /// до тормозного импульса разгоняем парусник
            if ( alpha > (-90. * M_PI / 180.) && alpha < (85. * M_PI / 180.) )
                vx += ( axe + axm + A ) * DT ;      /// если парус открыт, то ускорение А учитываем
            else
                vx += ( axe + axm ) * DT ;          /// если парус закрыт, то ускорение А не учитываем

/// после тормозного импульса летим по окололунной орбите
        else  /// пока летим по окололунной, пытаемся подтормозить парусом. Угол 70 взят наугад
            if ( alpha > 70. * M_PI / 180. )   vx += ( axe + axm + A ) * DT ;
            else                               vx += ( axe + axm ) * DT ;

        vy += ( aye + aym ) * DT ;              /// по оси Y солнечный ветер не давит
        v = sqrt ( vx*vx + vy*vy ) ;
        x += vx * DT ;                          /// пересчитываем координаты парусника
        y += vy * DT ;

        phie += omegaem * DT ;                  /// новые координаты Земли и Луны полярные
        phim += omegaem * DT ;
        if ( phie > M_PI ) phie -= 2. * M_PI ;  /// поддерживаем их в интервале от -180 до +180 градусов
        if ( phim > M_PI ) phim -= 2. * M_PI ;

        cos_phi = cos ( phie ) ;                /// немного экономлю на вызове синуса и косинуса,
        sin_phi = sin ( phie ) ;                /// т.к. Земля и Луна всегда находятся на противоположных углах
        xe = re * cos_phi ;
        ye = re * sin_phi  ;
        xm = rm * (-cos_phi) ;                  /// новые координаты Земли и Луны
        ym = rm * (-sin_phi) ;                  /// знаки синуса и косинуса переворачиваю

        xi_new = ( x*vx + y*vy ) ;              /// скалярное произведение радиус-вектора и векора скорости
                                                /// использую чтобы определить точку апогея
        if ( xi_new <= 0.0 && xi > 0.0 ) {      /// мы в апогее, засчитываем очередной виток в счётчик
            n++ ;                               /// счетчик витков
            if ( n % ticker == 0 ) report () ;
            }                                   /// виток завершён
        xi = xi_new ;

/// Приближается импульс двигателем, поэтому точки траектории начинаем писать почаще
        if ( i == IMPULSE - 2000000 ) ticker_orbit = TICKER_ORBIT_2 * 5 ;

/// На тике номер IMPULSE даём мгновенный тормозящий импульс двигателем.
/// Число 0.72 - коэффициент изменения скорости, просто подобран
#define ENGINE (0.72)
        if ( i == IMPULSE ) {
                report( ) ;
                printf ( "MOON COORDINATES: x=%.2lf\ty=%.2lf\n" , xm/1.e6, ym/1.e6 ) ;
                printf ( "BEFORE IMPULSE: v = %.2lf m/s\n" , v = sqrt ( vx*vx + vy*vy ) ) ;
                printf ( "ENGINE IMPULSE: dv = %.2lf\n" , v * ( 1. - ENGINE )) ;
                vx *= ENGINE ;         /// умножаем скорость на коэффициент ENGINE
                vy *= ENGINE ;
                printf ( "AFTER IMPULSE: v = %.2lf m/s\n" , v = sqrt ( vx*vx + vy*vy )  ) ;
                report ( ) ;
                ticker_orbit = TICKER_ORBIT_2  ; /// далее орбиту будем писать чаще
        }

        if ( n == 420 ) break ;                 /// прекращаем полёт, миссия выполнена
        r0m_min = min ( r0m, r0m_min ) ;        /// минимальное расстояние до центра Луны -
                                                /// контроль на столкновение с Луной
                                                /// делаем на каждом тике DT
        t += DT ;
    }  /// конец цикла, идём на следующий шаг DT

    fclose ( fo ) ;

/// Выводим на экран время полёта (витков и дней)
    printf ( "\nRounds done = %d\nFlight time = %.0lf days\n", n, t / (60.*60.*24.) ) ;

/// контроль минимальной высоты над поверхностью Луны ведётся в течение всего полёта.
/// Если число положительное, то столкновения не было. Получаем +577 км => всё норм
    printf ( "\nMin H above Moon = %+5.0lf km\n", (r0m_min - Rm) / 1.e3 ) ;

    return 0 ;
}
