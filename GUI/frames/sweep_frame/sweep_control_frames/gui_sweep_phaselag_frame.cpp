#include "gui_sweep_phaselag_frame.h"
#include "../gui_sweep_frame.h"

namespace RhodotronSimulatorGUI::frames::subframes{


    PhaseLagSweepControlFrame::PhaseLagSweepControlFrame(const TGWindow* p, UInt_t w, UInt_t h, 
        Analysis::Analyzer* analyzer_, GUISimulationHandler* sim_handler_, data::DataProvider* dataProvider_, ConfigurationFrame* config_frame_)
         : SweepControlVerticalFrame(p, w, h, analyzer_, sim_handler_, dataProvider_, config_frame_){

        _phase_lag_data_mutex = std::make_shared<std::mutex>();
        _data_updated_mutex = std::make_shared<std::mutex>();
        _worker_terminate = std::make_shared<bool>();
        _data_updated = std::make_shared<bool>();

        _phase_vs_muE_graph = new TGraph();
        _phase_vs_sigmaE_graph = new TGraph();
        _phase_vs_sigmaR_graph = new TGraph();

        auto input_frame = new TGHorizontalFrame(this);
        auto start_frame = new TGVerticalFrame(input_frame);
        auto end_frame = new TGVerticalFrame(input_frame);

        // Setup the start label and input 
        phase_lag_sweep_start_label = new TGLabel(start_frame, Sweep_phaselag_start_label_text.c_str());
        phase_lag_sweep_start_label->SetTextFont(FONT_BOLD_12);

        phase_lag_sweep_start = new TGNumberEntry(start_frame,0.0,4, -1, TGNumberFormat::kNESInteger, 
                                TGNumberFormat::kNEAAnyNumber, TGNumberFormat::kNELLimitMinMax, -180, 180);
        phase_lag_sweep_start->Resize(100, 25);

        start_frame->AddFrame(phase_lag_sweep_start_label, center_x_layout);
        start_frame->AddFrame(phase_lag_sweep_start, center_x_layout);

        // Setup the end label and input 
        phase_lag_sweep_end_label = new TGLabel(end_frame, Sweep_phaselag_end_label_text.c_str());
        phase_lag_sweep_end_label->SetTextFont(FONT_BOLD_12);

        phase_lag_sweep_end = new TGNumberEntry(end_frame,0.0,4, -1, TGNumberFormat::kNESInteger, 
                                TGNumberFormat::kNEAAnyNumber, TGNumberFormat::kNELLimitMinMax, -180, 180);
        phase_lag_sweep_end->Resize(100, 25);

        end_frame->AddFrame(phase_lag_sweep_end_label, center_x_layout);
        end_frame->AddFrame(phase_lag_sweep_end, center_x_layout);
        
        input_frame->AddFrame(start_frame, center_y_layout);
        input_frame->AddFrame(end_frame, center_y_layout);

        current_phase = new TGLabel(this, " ");

        auto sweep_button_frame = new TGHorizontalFrame(this);

        _sweep_button = new TGTextButton(sweep_button_frame, Sweep_sweep_button_text.c_str());
        _sweep_button->Connect("Clicked()", "RhodotronSimulatorGUI::frames::subframes::PhaseLagSweepControlFrame", this, 
                             "PhaselagSweepRequested()");

        _stop_button = new TGTextButton(sweep_button_frame, Sweep_stop_button_text.c_str());
        _stop_button->Connect("Clicked()", "RhodotronSimulatorGUI::frames::subframes::PhaseLagSweepControlFrame", this, 
                             "PhaselagSweepStopRequested()");
        _stop_button->SetEnabled(false);   

        sweep_button_frame->AddFrame(_sweep_button, center_x_layout);            
        sweep_button_frame->AddFrame(_stop_button, center_x_layout);    

        auto draw_button_frame = new TGHorizontalFrame(this);

        _draw_phase_vs_muE_button = new TGTextButton(draw_button_frame, "Draw phase vs muE");
        _draw_phase_vs_muE_button->Connect("Clicked()", "RhodotronSimulatorGUI::frames::subframes::PhaseLagSweepControlFrame", this, 
                             "DrawPhaseLagvsAverageE()");
        _draw_phase_vs_muE_button->SetEnabled(false);
        
        _draw_phase_vs_sigmaE_button = new TGTextButton(draw_button_frame, "Draw phase vs sigmaE");
        _draw_phase_vs_sigmaE_button->Connect("Clicked()", "RhodotronSimulatorGUI::frames::subframes::PhaseLagSweepControlFrame", this, 
                             "DrawPhaseLagvsStdE()");
        _draw_phase_vs_sigmaE_button->SetEnabled(false);

        _draw_phase_vs_sigmaR_button = new TGTextButton(draw_button_frame, "Draw phase vs sigmaR");
        _draw_phase_vs_sigmaR_button->Connect("Clicked()", "RhodotronSimulatorGUI::frames::subframes::PhaseLagSweepControlFrame", this, 
                             "DrawPhaseLagvsStdR()");
        _draw_phase_vs_sigmaR_button->SetEnabled(false);

        draw_button_frame->AddFrame(_draw_phase_vs_muE_button, center_x_layout);
        draw_button_frame->AddFrame(_draw_phase_vs_sigmaE_button, center_x_layout);
        draw_button_frame->AddFrame(_draw_phase_vs_sigmaR_button, center_x_layout);


        this->AddFrame(current_phase, center_layout_notoppadding);
        this->AddFrame(progressBar, center_layout_notoppadding);
        this->AddFrame(input_frame, center_layout_notoppadding);
        this->AddFrame(canvas, center_layout);
        this->AddFrame(sweep_button_frame, center_layout);
        this->AddFrame(draw_button_frame, center_layout);

        _draw_timer = new TTimer(1000);
        _draw_timer->Connect("Timeout()", "RhodotronSimulatorGUI::frames::subframes::PhaseLagSweepControlFrame", this, "DrawActiveGraphIfThereIsUpdate()");
    }


    void PhaseLagSweepControlFrame::SetCurrentPhase(double phase){
        char temp[10];

        snprintf(temp, 10, "%.1lf", phase);

        std::string description = "Simulating for phase lag : ";
        description += temp;
        description += " degrees";

        current_phase->SetText(description.c_str());
        this->ShowFrame(current_phase);
    }

    void PhaseLagSweepControlFrame::PhaselagSweepRequested(){
        PhaseLagSweepWorkerArguments worker_args;

        double start= phase_lag_sweep_start->GetNumber();
        worker_args.start_phase = start;
        worker_args.end_phase = phase_lag_sweep_end->GetNumber();
        worker_args.analyzer = analyzer;
        worker_args.dataProvider = dataProvider;
        worker_args.config_frame = config_frame;
        worker_args.sim_handler = sim_handler;
        worker_args.data = &phaselagData;
        worker_args.dataMutex = _phase_lag_data_mutex;
        worker_args.terminate = _worker_terminate;

        worker_args.dataUpdated = _data_updated;
        worker_args.dataMutex = _data_updated_mutex;
        worker_args.owner = this;

        if ( worker != NULL){ 
            if(worker->joinable()){
                worker->join();
            }
            delete worker;
        }
        *(_worker_terminate.get()) = false;
        worker = new std::thread(PhaseLagSweepControlFrame::PhaselagSweepWork, worker_args);
    }

    void PhaseLagSweepControlFrame::PhaselagSweepStopRequested(){
        if ( worker != NULL){ 
            *(_worker_terminate.get()) = true;
            if(worker->joinable()){
                worker->join();
            }
            delete worker;
        }
    }

    void PhaseLagSweepControlFrame::PhaselagSweepWork(PhaseLagSweepWorkerArguments args){
        
        args.owner->SweepingModeActivate(true);

        args.data->clear();
        args.sim_handler->set_progress_bar(args.owner->GetProgressBar());
        args.sim_handler->set_status_label(NULL);

        
        int phaselag_start = args.start_phase;
        int phaselag_end = args.end_phase;

        if ( phaselag_start > phaselag_end ){
            int temp = phaselag_end;
            phaselag_end = phaselag_start;
            phaselag_start = temp;
        }

        for ( int ph = phaselag_start ; ph <= phaselag_end ; ph++){
            if(* args.terminate.get()){
                break;
            }
            args.owner->SetCurrentPhase(ph);

            args.config_frame->SetPhaseLag(ph);
            args.config_frame->SaveConfigPressed();

            args.sim_handler->spawn_server(true, false);
            args.sim_handler->spawn_simulation();

            args.dataProvider->clearLogs();

            while ( !args.sim_handler->server_joinable() ){
                std::this_thread::yield();
            }
            args.sim_handler->join_server();

            args.dataProvider->fillLogs();

            PhaseLagSweepData data;
            data.phase = ph;
            data.muE = args.analyzer->GetAverageE();
            data.sigmaE = args.analyzer->GetStdDevE();
            data.sigmaR = args.analyzer->GetStdDevR();

            args.dataMutex->lock();
            args.data->push_back(data);
            args.dataMutex->unlock();

            args.dataMutex->lock();
            (*args.dataUpdated) = true;
            args.dataMutex->unlock();
        }

        args.owner->SweepingModeActivate(false);
    }

    void PhaseLagSweepControlFrame::SweepingModeActivate(bool active){
        _sweeping_mode_active = active;
        if(active){
            //this->HideFrame(canvas);
            this->ShowFrame(progressBar);
            this->ShowFrame(current_phase);

            canvas->Resize(400,400);
            canvas->GetCanvas()->Resize();
            
            _sweep_button->SetEnabled(false);
            _stop_button->SetEnabled(true);
            
            phase_lag_sweep_start->SetState(false);
            phase_lag_sweep_end->SetState(false);
            
            _draw_phase_vs_muE_button->SetEnabled(true);
            _draw_phase_vs_sigmaE_button->SetEnabled(true);
            _draw_phase_vs_sigmaR_button->SetEnabled(true);

            _draw_timer->TurnOn();
            
            this->Layout();
            parent->Layout();
        }
        else{
            //this->ShowFrame(canvas);
            this->HideFrame(progressBar);
            this->HideFrame(current_phase);

            _draw_timer->TurnOff();

            canvas->Resize(500,500);
            canvas->GetCanvas()->Resize();

            _sweep_button->SetEnabled(true);
            _stop_button->SetEnabled(false);
            
            phase_lag_sweep_start->SetState(true);
            phase_lag_sweep_end->SetState(true);
            
            _draw_phase_vs_muE_button->SetEnabled(true);
            _draw_phase_vs_sigmaE_button->SetEnabled(true);
            _draw_phase_vs_sigmaR_button->SetEnabled(true);
            
            this->Layout();
            parent->Layout();
        }
    }

    void PhaseLagSweepControlFrame::DrawPhaseLagvsAverageE(){
        _active_graph = muE_vs_phase;
        this->ShowFrame(canvas);
        if(!_sweeping_mode_active){
            this->HideFrame(progressBar);
            this->HideFrame(current_phase);
        }
        this->Layout();
        parent->Layout();

        TGraph* muGraph = _phase_vs_muE_graph;
        muGraph->Clear();

        _phase_lag_data_mutex->lock();
        float maxE, maxphase;
        for (int i = 0 ; i < phaselagData.size() ; i++){
            if ( maxE < phaselagData[i].muE){
                maxE = phaselagData[i].muE;
                maxphase = phaselagData[i].phase;
            }
            muGraph->AddPoint(phaselagData[i].phase, phaselagData[i].muE);
        }
        _phase_lag_data_mutex->unlock();

        canvas->GetCanvas()->Clear();
        canvas->GetCanvas()->cd();
        canvas->GetCanvas()->SetGrid();
        canvas->GetCanvas()->SetCrosshair();
        canvas->GetCanvas()->SetMargin(0.135, 0.065, 0.1, 0.1);

        muGraph->SetTitle("#mu_{E} vs #phi_{lag}");

        muGraph->GetXaxis()->SetTitle("#phi_{lag} (degree)");
        muGraph->GetXaxis()->SetLabelSize(0.03);
        muGraph->GetXaxis()->SetTitleOffset(1.25);

        muGraph->GetYaxis()->SetTitle("#mu_{E} (MeV)");
        muGraph->GetYaxis()->SetLabelSize(0.03);
        muGraph->GetYaxis()->SetTitleOffset(1.75);

        muGraph->SetMarkerStyle(21);
        muGraph->SetMarkerColor(kBlue);
        muGraph->Draw("ap");

        canvas->GetCanvas()->Update();
        canvas->GetCanvas()->Modified();
    }

    void PhaseLagSweepControlFrame::DrawPhaseLagvsStdE(){
        _active_graph = sE_vs_phase;
        this->ShowFrame(canvas);
        if(!_sweeping_mode_active){
            this->HideFrame(progressBar);
            this->HideFrame(current_phase);
        }
        this->Layout();
        parent->Layout();

        TGraph* sGraph = _phase_vs_sigmaE_graph;
        sGraph->Clear();

        _phase_lag_data_mutex->lock();
        for (int i = 0 ; i < phaselagData.size() ; i++){
            sGraph->AddPoint(phaselagData[i].phase, phaselagData[i].sigmaE);
        }
        _phase_lag_data_mutex->unlock();

        canvas->GetCanvas()->Clear();
        canvas->GetCanvas()->cd();
        canvas->GetCanvas()->SetGrid();
        canvas->GetCanvas()->SetCrosshair();
        canvas->GetCanvas()->SetMargin(0.135, 0.065, 0.1, 0.1);

        sGraph->SetTitle("#sigma_{E} vs #phi_{lag}");

        sGraph->GetXaxis()->SetTitle("#phi_{lag} (degree)");
        sGraph->GetXaxis()->SetLabelSize(0.03);
        sGraph->GetXaxis()->SetTitleOffset(1.25);

        sGraph->GetYaxis()->SetTitle("#sigma_{E} (MeV)");
        sGraph->GetYaxis()->SetLabelSize(0.03);
        sGraph->GetYaxis()->SetTitleOffset(1.75);

        sGraph->SetMarkerStyle(20);
        sGraph->SetMarkerColor(kRed);

        sGraph->Draw("ap");

        canvas->GetCanvas()->Update();
        canvas->GetCanvas()->Modified();
    }

    void PhaseLagSweepControlFrame::DrawPhaseLagvsStdR(){
        _active_graph = sR_vs_phase;
        this->ShowFrame(canvas);
        if(!_sweeping_mode_active){
            this->HideFrame(progressBar);
            this->HideFrame(current_phase);
        }
        this->Layout();
        parent->Layout();

        TGraph* sRGraph = _phase_vs_sigmaR_graph;
        sRGraph->Clear();

        _phase_lag_data_mutex->lock();
        for (int i = 0 ; i < phaselagData.size() ; i++){
            sRGraph->AddPoint(phaselagData[i].phase, phaselagData[i].sigmaR);
        }
        _phase_lag_data_mutex->unlock();

        canvas->GetCanvas()->Clear();
        canvas->GetCanvas()->cd();
        canvas->GetCanvas()->SetGrid();
        canvas->GetCanvas()->SetCrosshair();
        canvas->GetCanvas()->SetMargin(0.135, 0.065, 0.1, 0.1);

        sRGraph->SetTitle("#sigma_{r} vs #phi_{lag}");

        sRGraph->GetXaxis()->SetTitle("#phi_{lag} (degree)");
        sRGraph->GetXaxis()->SetLabelSize(0.03);
        sRGraph->GetXaxis()->SetTitleOffset(1.25);
        
        sRGraph->GetYaxis()->SetTitle("#sigma_{r} (m)");
        sRGraph->GetYaxis()->SetLabelSize(0.03);
        sRGraph->GetYaxis()->SetTitleOffset(1.75);

        sRGraph->SetMarkerStyle(20);
        sRGraph->SetMarkerColor(kAlpine);

        sRGraph->Draw("ap");

        canvas->GetCanvas()->Update();
        canvas->GetCanvas()->Modified();

        this->Layout();
    }

    void PhaseLagSweepControlFrame::DrawActiveGraph(){
        
        switch (_active_graph)
        {
        case muE_vs_phase:
            DrawPhaseLagvsAverageE();
            break;
        case sE_vs_phase:
            DrawPhaseLagvsStdE();
            break;
        case sR_vs_phase:
            DrawPhaseLagvsStdR();
            break;
        case None:
            break;
        }
    }

    void PhaseLagSweepControlFrame::DrawActiveGraphIfThereIsUpdate(){
        if(_data_updated){
            DrawActiveGraph();
            _data_updated_mutex->lock();
            *_data_updated = false;
            _data_updated_mutex->unlock();
        }
    }

    void PhaseLagSweepControlFrame::OnNavigatedTo(){
        HideFrame(canvas);
        HideFrame(progressBar);
        parent->Layout();
    }

    void PhaseLagSweepControlFrame::OnNavigatedFrom(){

    }

}