/* -*- C++ -*- */
#ifndef RSITE_ARDUINO_MENU_SYSTEM_IO
  #define RSITE_ARDUINO_MENU_SYSTEM_IO

  #include "menuBase.h"
  #include "shadows.h"
  
  namespace Menu {
    ////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    // Output
    ////////////////////////////////////////////////////////////////////////////

    //navigation panels (min 1) describe output dimensions (in characters)
    struct panel {
      idx_t x,y,w,h;
      inline idx_t maxX() const {return x+w;}
      inline idx_t maxY() const {return y+h;}
    };

    class panelsList {
      public:
        constMEM panel* panels;
        navNode** nodes;
        constMEM idx_t sz;
        idx_t cur=0;
        panelsList(constMEM panel p[],navNode* nodes[],idx_t sz):panels(p),nodes(nodes),sz(sz) {
          reset();
        }
        void reset(idx_t from=0) {
          for(int n=from;n<sz;n++) nodes[n]=NULL;
        }
        inline constMEM panel operator[](idx_t i) const {
          assert(i<sz);
          #ifdef USING_PGM
            panel tmp;
            memcpy_P(&tmp, &panels[i], sizeof(panel));
            return tmp;
          #else
            return panels[i];
          #endif
        }
        idx_t maxX() const {
          idx_t r=0;
          for(int n=0;n<sz;n++) r=_MAX(operator[](n).maxX(),r);
          return r;
        }
        idx_t maxY() const {
          idx_t r=0;
          for(int n=0;n<sz;n++) r=_MAX(operator[](n).maxY(),r);
          return r;
        }
    };

    ///////////////////////////////////////////////////////////////////////////
    // base for all menu input devices
    class menuIn:public Stream {
      public:
        bool numValueInput=true;
        virtual void setFieldMode(bool) {}
        virtual bool fieldMode() const {return false;}
        inline void fieldOn() {setFieldMode(true);}
        inline void fieldOff() {setFieldMode(false);}
    };

    #ifdef MENU_ASYNC
      class StringStream:public menuIn {
        public:
          const char *src;
          StringStream(const char*s):src(s) {}
          int available() override {return 0!=*src;}
          int read() override {return *src++;}
          int peek() override {return *src?*src:-1;}
          void flush() override {while(*src) src++;}
          size_t write(uint8_t) override {return 0;}
          operator const String() {return String(src);}
      };
    #endif

    ///////////////////////////////////////////////////////////////////////////
    // base for all menu output devices
    class menuOut:public Print {
      friend class prompt;
      public:
        idx_t* tops;
        panelsList& panels;
        idx_t lastSel=-1;
        //TODO: turn this bool's into bitfield flags
        enum styles {none=0<<0,redraw=1<<0,minimalRedraw=1<<1, drawNumIndex=1<<2, usePreview=1<<3, expandEnums=1<<4,rasterDraw=1<<5} style;
        enum fmtParts {fmtPanel,fmtTitle,fmtBody,fmtOp,fmtIdx,fmtCursor,fmtOpBody,fmtPreview,fmtPrompt,fmtField,fmtToggle,fmtSelect,fmtChoose,fmtUnit};
        menuNode* drawn=NULL;
        menuOut(idx_t *topsList,panelsList &p,styles os=minimalRedraw)
          :tops(topsList),panels(p),style(os) {}
        inline idx_t maxX(idx_t i=0) const {return panels[i].w;}
        inline idx_t maxY(idx_t i=0) const {return panels[i].h;}
        inline idx_t& top(navNode& nav) const;
        idx_t printRaw(const char* at,idx_t len);
        #ifdef DEBUG
          virtual menuOut& operator<<(prompt const &p);
          #ifdef ESP8266
            template<typename T> menuOut& operator<<(T o) {(*(Print*)this)<<(o);return *this;}
          #endif
        #endif
        virtual menuOut& fill(
          int x1, int y1, int x2, int y2,char ch=' ',
          colorDefs color=bgColor,
          bool selected=false,
          status stat=enabledStatus,
          bool edit=false
        ) {return *this;}
        void clearChanged(navNode &nav);//clean up changed flags after everyone printed!
        void previewMenu(navRoot& root,menuNode& menu,idx_t panelNr);//draw a preview on a panel
        Used printMenu(navNode &nav);//print menus and previews on panels
        virtual void clearLine(idx_t ln,idx_t panelNr=0,colorDefs color=bgColor,bool selected=false,status stat=enabledStatus,bool edit=false)=0;
        virtual void clear()=0;
        virtual void clear(idx_t panelNr)=0;
        virtual void setCursor(idx_t x,idx_t y,idx_t panelNr=0)=0;
        virtual void setColor(colorDefs c,bool selected=false,status s=enabledStatus,bool edit=false) {}
        virtual void drawCursor(idx_t ln,bool selected,status stat,bool edit=false,idx_t panelNr=0) {
          setColor(cursorColor, selected, stat,edit);
          write(selected?(stat==disabledStatus? options->disabledCursor : options->selectedCursor):' ');
        }
        void doNav(navCmd cmd,navNode &nav);
        #ifdef MENU_FMT_WRAPS
          virtual result fmtStart(fmtParts part,navNode &nav,idx_t idx=-1) {return proceed;}
          virtual result fmtEnd(fmtParts part,navNode &nav,idx_t idx=-1) {return proceed;}
        #endif
        //text editor cursors
        virtual idx_t startCursor(navRoot& root,idx_t x,idx_t y,bool charEdit,idx_t panelNr=0) {write(charEdit?">":"[");return 1;}
        virtual idx_t endCursor(navRoot& root,idx_t x,idx_t y,bool charEdit,idx_t panelNr=0) {write(charEdit?"<":"]");return 1;}
        virtual idx_t editCursor(navRoot& root,idx_t x,idx_t y,bool editing,bool charEdit,idx_t panelNr=0) {return 0;}
        virtual void rect(idx_t panelNr,idx_t x,idx_t y,idx_t w=1,idx_t h=1,colorDefs c=bgColor,bool selected=false,status stat=enabledStatus,bool edit=false) {}
        virtual void box(idx_t panelNr,idx_t x,idx_t y,idx_t w=1,idx_t h=1,colorDefs c=bgColor,bool selected=false,status stat=enabledStatus,bool edit=false) {}
      protected:
        Used printMenu(navNode &nav,idx_t panelNr);
    };

    //inline menuOut::styles operator | (menuOut::styles a, menuOut::styles b) {return (menuOut::styles)(a|b);}

    //for devices that can position a print cursor (like LCD's)
    class cursorOut:public menuOut {
    public:
      cursorOut(idx_t *topsList,panelsList &p,styles os=minimalRedraw)
        :menuOut(topsList,p,os) {}
      void clearLine(idx_t ln,idx_t panelNr=0,colorDefs color=bgColor,bool selected=false,status stat=enabledStatus,bool edit=false) override {
        setCursor(0,ln,panelNr);
        for(int n=0;n<maxX();n++) print(' ');
        setCursor(0,ln,panelNr);
      }
      void clear(idx_t panelNr) override {
        const panel p=panels[panelNr];
        fill(p.x,p.y,p.x+p.w-1,p.y+p.h-1);
        setCursor(0,0,panelNr);
        panels.nodes[panelNr]=NULL;
      }
      menuOut& fill(
        int x1, int y1, int x2, int y2,char ch=' ',
        colorDefs color=bgColor,
        bool selected=false,
        status stat=enabledStatus,
        bool edit=false
      ) override {
        for(int r=y1;r<=y2;r++) {
          setCursor(x1,r);
          for(int c=x1;c<=x2;c++)
            write(ch);
        }
        return *this;
      }
    };

    class gfxOut:public menuOut {
      public:
        idx_t resX=1;
        idx_t resY=1;
        idx_t fontMarginY=1;//in pixels, compensate vertical font alignment
        gfxOut(idx_t rx,idx_t ry,idx_t* t,panelsList &p,menuOut::styles st=menuOut::minimalRedraw,idx_t fontMarginY=1)
          :menuOut(t,p,st),resX(rx),resY(ry) {}
        idx_t startCursor(navRoot& root,idx_t x,idx_t y,bool charEdit,idx_t panelNr) override {
          if (charEdit) {
            rect(panelNr,  x,  y, 1, 1, bgColor, false, enabledStatus, false);
            setColor(fgColor,false,enabledStatus,false);
          } else
            box(panelNr,  x,  y, 1, 1, bgColor, false, enabledStatus, false);
          return 0;
        }
        idx_t endCursor(navRoot& root,idx_t x,idx_t y,bool charEdit,idx_t panelNr) override {
          setColor(fgColor,true,enabledStatus,true);return 0;
        }
        idx_t editCursor(navRoot& root,idx_t x,idx_t y,bool editing,bool charEdit,idx_t panelNr) override {return 0;}
    };

    //list of output devices
    //this allows parallel navigation on multiple devices
    class outputsList {
      public:
        int cnt=1;
        menuOut* constMEM* outs;
        outputsList(menuOut* constMEM o[],int n):cnt(n),outs(o) {}
        menuOut& operator[](idx_t i) {
          assert(i<cnt);
          return *(menuOut*)memPtr(outs[i]);
        }
        Used printMenu(navNode& nav) const;
        void refresh() {//force redraw of all outputs on next output call
          for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->drawn=NULL;
        }
        // void clearLine(idx_t ln,idx_t panelNr=0,colorDefs color=bgColor,bool selected=false,status stat=enabledStatus) const {
        //   for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->clearLine(ln,panelNr,color,selected,stat);
        // }
        void clearChanged(navNode& nav) const {
          for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->clearChanged(nav);
        }
        void clear() {for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->clear();}
        // void setCursor(idx_t x,idx_t y) {
        //   for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->setCursor(x,y);
        // }
        // void setColor(colorDefs c,bool selected=false,status s=enabledStatus) {
        //   for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->setColor(c,selected,s);
        // }
        // void drawCursor(idx_t ln,bool selected,status stat,idx_t panelNr=0) {
        //   for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->drawCursor(ln,selected,stat,panelNr);
        // }
        void doNav(navCmd cmd,class navNode &nav) {for(int n=0;n<cnt;n++) ((menuOut*)memPtr(outs[n]))->doNav(cmd,nav);}
        result idle(idleFunc f,idleEvent e) {
          #ifdef DEBUG
          if (!f) Serial<<"idleFunc is NULL!!!"<<endl;
          #endif
          if (!f) return proceed;
          for(int n=0;n<cnt;n++) {
            menuOut& o=*((menuOut*)memPtr(outs[n]));
            switch(e) {
              case idleStart:
                if ((*f)(o,e)==proceed) {
                  if (!(o.style&menuOut::redraw)) {
                    result r=(*f)(o,idling);
                    if (r==quit) return r;
                  }
                } else return quit;
                break;
              case idling:
                if (o.style&menuOut::redraw) {
                  result r=(*f)(o,e);
                  if (r==quit) return r;
                }
                break;
              case idleEnd:
                result r=(*f)(o,e);
                if (r==quit) return r;
                break;
            }
          }
          return proceed;
        }
    };

    // input
    ////////////////////////////////////////////////////////////////////////////

  }
#endif