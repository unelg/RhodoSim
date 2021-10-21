#include <math.h>
#include <vector>
#include <iostream>
#include <iomanip>
#include "rhodo.h"
using namespace std;


//r is in mm
//time is in ns
//phase in radians
double Eradial(double r, double time, double phase){
  double w=2*M_PI*freq/1000;

  double E_zero = sin(w*time+phase)*Emax*Emax_pos/r;

  double res = E_zero*((r < -R1*1000)*(r > -R2*1000) | (r < R2*1000)*(r > R1*1000));

  return res;
}

double vel_to_dist(double vel, double t){
  return vel*t;
}

double dist_to_time(double dist, double vel){
  return dist/vel;
}

double bir_gecis(double r_pos, double Et, double t){
  for(; t<SimuTime; t+=dT){
    double vel = c*sqrt(Et*Et-E0*E0)/Et;
    double RelBeta  = vel/c;
    double RelGamma = 1.0 / sqrt(1.0-RelBeta*RelBeta);

    double ef=Eradial(r_pos*1000,t,RFphase*deg_to_rad); // convert position to mm
    double acc=ef*1E6*eQMratio/(RelGamma*RelGamma*RelGamma); 

    r_pos = r_pos + vel * dT*ns + 1/2*acc*(dT*ns)*(dT*ns);
    vel=vel+acc*dT*ns;
    RelBeta  = vel/c;
    RelGamma = 1.0 / sqrt(1.0-RelBeta*RelBeta);
    Et=RelGamma*E0; 
  }
  return Et;
}

double gecis(double r_pos, double Et, double &t){

  for(; r_pos >= -R2 && r_pos <= R2 ; t+=dT){
    double vel = c*sqrt(Et*Et-E0*E0)/Et;
    double RelBeta  = vel/c;
    double RelGamma = 1.0 / sqrt(1.0-RelBeta*RelBeta);

    double ef=Eradial(r_pos*1000,t,RFphase*deg_to_rad); // convert position to mm
    double acc=ef*1E6*eQMratio/(RelGamma*RelGamma*RelGamma); 

    r_pos = r_pos + vel * dT*ns + 1/2*acc*(dT*ns)*(dT*ns);
    vel=vel+acc*dT*ns;
    RelBeta  = vel/c;
    RelGamma = 1.0 / sqrt(1.0-RelBeta*RelBeta);
    Et=RelGamma*E0; 
  }
  return Et;
}


double Electron::get_vel(){
    return c*sqrt(Et*Et-E0*E0)/Et;
}

double Electron::get_travel_time(double dist){
    return dist/get_vel();
}

void Electron::print_electron_info(){
    cout<<std::setprecision(4);
    for(int i = 0; i < t_giris_cikis.size() ; i++ ){
        cout << "\tGecis " << i+1 << ") " << "Enerji : " << enerjiler.at(i)-E0 << " MeV, giris zamani : " << t_giris_cikis.at(i).first << " ns, cikis zamani : " << t_giris_cikis.at(i).second << " ns" << endl;
    }
}

void Electron::e_gecis(double &t){
    for(; r_pos >= -R2 && r_pos <= R2 ; t+=dT){
        double vel = get_vel();
        double RelBeta  = vel/c;
        double RelGamma = 1.0 / sqrt(1.0-RelBeta*RelBeta);

        double ef=Eradial(this->r_pos*1000,t,RFphase*deg_to_rad); // convert position to mm
        double acc=ef*1E6*eQMratio/(RelGamma*RelGamma*RelGamma); 

        r_pos += vel * dT*ns + 1/2*acc*(dT*ns)*(dT*ns);
        vel=vel+acc*dT*ns;
        RelBeta  = vel/c;
        RelGamma = 1.0 / sqrt(1.0-RelBeta*RelBeta);
        Et=RelGamma*E0; 
    }
    enerjiler.push_back(Et);
}




void Bunch::reset_pos(){
    for(int i = 0; i < e_count ; i++){
        e[i].r_pos = -R2;
    }
}

void Bunch::print_bunch_info(){
    cout<<std::setprecision(4);
    for(int i = 0; i < e_count;i++){
        if( i == index_fastest ){
        cout << "**\t";
        }
        cout << "Electron " << i+1 << ":" << endl;
        e[i].print_electron_info();
    }
}

void Bunch::bunch_gecis(double &t_delay_of_max){
    bool isFirstPass = this->pass_count == 0;
    isFirstPass ? this->bunch_ilk_gecis(t_delay_of_max) : this->bunch_nth_gecis(t_delay_of_max);
}

void Bunch::bunch_ilk_gecis(double &t){
    double t_e[e_count];
    giris_cikis_tpair tpair;
    double emax = 0;
    for(int i = 0 ; i < e_count ; i++){
        t_e[i] = t + i * ns_between;
        tpair.first = t_e[i];
        e[i].e_gecis(t_e[i]);
        tpair.second = t_e[i];
        e[i].t_giris_cikis.push_back(tpair);
        if(emax < e[i].Et){
            emax = e[i].Et;
            index_fastest = i;
        }
    }
    pass_count++;
}

// ilk geçiş için bu kullanılmayacak. for loop içerisinde pass_count kontrolü yok
void Bunch::bunch_nth_gecis(double t_delay_of_max){
  double t_e[e_count];
  giris_cikis_tpair tpair;
  double emax = 0;
  // find the distance bunch needs to travel out of the cavity given the delay of the peak energy electron
  double dist_out = vel_to_dist(e[index_fastest].get_vel(), t_delay_of_max);

  for(int i = 0 ; i < e_count ; i++){
    // bir elektronun giriş zamanı = bir öncekinden çıkış zamanı + dışarıdaki yolda geçirdiği zaman
    t_e[i] = e[i].t_giris_cikis.at(pass_count-1).second + e[i].get_travel_time(dist_out);
    tpair.first = t_e[i];
    e[i].e_gecis(t_e[i]);
    tpair.second = t_e[i];
    e[i].t_giris_cikis.push_back(tpair);
    if(emax < e[i].Et){
      emax = e[i].Et;
      index_fastest = i;
    }
  }
  pass_count++;
}