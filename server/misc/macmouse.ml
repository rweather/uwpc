; $Header: /c/cak/lib/mlisp/RCS/macmouse.ml,v 1.5 85/11/05 14:01:44 cak Rel $
; 
; Macintosh mouse routines for use with John Bruner's uw program.
; 	Chris Kent, Purdue University Fri Oct 25 1985
; 	Copyright 1985 by Christopher A. Kent. All rights reserved.
; 	Permission to copy is given provided that the copy is not
; 	sold and this copyright notice is included.
; 
; Provides a scroll bar/thumbing area in the unused scroll bar with the
; following features:
; 	click at line 1 does previous page
;	click at line 24 does next page
; 	click anywhere else "thumbs" to the relative portion of the buffer.
; 	shift-click at line 1 scrolls one line down
; 	shift-click at line 24 scrolls one line up
; 	shift-click elsewhere moves line to top of window
; 	option-shift-click elsewhere moves line to bottom of window
; 
; There is also basic positioning and kill-buffer support:
; 	click in a buffer moves dot there
; 	drag copies the dragged region to the kill buffer (mark is left
; 		at the beginning of the region.)
; 	shift-drag deletes the dragged region to the kill buffer
;   it is possible to use the scrolling and thumbing area to make the region
;   larger than a single screen; just click, scroll, release. Make sure
;   that the last scroll is just a down event; the up must be in the buffer.
;
; 	option-click yanks from the kill buffer, doesn't affect mark.
; 	option-shift-click similarly yanks from a named buffer.
; 

(declare-global
    #mouse-last-x		; x of last event
    #mouse-last-y		; y of last event
    #mouse-last-b		; buttons at last event
    #mouse-last-dot		; dot after last event
    #mouse-last-action		; whether last was scroll (1) or edit (2)
)

(defun
    (move-mac-cursor savest b x y up down lock shift option command saveb
	(setq savest stack-trace-on-error)
	(setq stack-trace-on-error 0)
	; decode everything
	(setq y (- (get-tty-character) 32))
	(setq x (- (get-tty-character) 32))
	(setq b (- (get-tty-character) 32))
	(setq saveb b)
	(setq command (% b 2))(setq b (/ b 2))	; command key
	(setq shift (% b 2))(setq b (/ b 2))	; shift 
	(setq lock (% b 2))(setq b (/ b 2))	; caps-lock
	(setq option (% b 2))(setq b (/ b 2))	; option
	(setq down (% b 2))(setq b (/ b 2))	; mouse down
	(setq up (% b 2))
	
	(if (= x 81)		; right margin -- move-dot-to-x-y is wrong
	    (progn 
		   (#mouse-scroll-region)
		   (setq #mouse-last-action 1))
	    (if (error-occurred 
		    (if (= #mouse-last-action 2)	; not if just scrolled
			(setq #mouse-last-dot (dot)))
		    (move-dot-to-x-y x y)
		    (backward-character)(forward-character)
		    (#mouse-edit-action)
		    (setq #mouse-last-action 2)
		)
		(progn 
		       (#mouse-scroll-region b x y)
		       (setq #mouse-last-action 1))
	    ))
	(setq stack-trace-on-error savest)
	(if (= down 1)
	    (progn 
		   (setq #mouse-last-x x)
		   (setq #mouse-last-y y)
		   (setq #mouse-last-b saveb))
	    (progn 
		   (setq #mouse-last-x 0)
		   (setq #mouse-last-y 0)
		   (setq #mouse-last-b 0)))
    )
    
    (#mouse-edit-action		; marking and editing actions on buttons:
				;   if no movement, nothing.
				;   if movement, put  mark at #mouse-last-dot,
				;      leave dot here,and edit.
				; editing (on upstrokes):
				;   unmodified, copy to kill buffer.
				;   SHIFTed, delete (cut) to kill buffer.
				; 
				; option-click yanks from kill buffer; 
				; shift-option-click from named buffer.
	(if (= saveb 16)
	    (#mouse-d))
	(if (= saveb 17)
	    (#mouse-dc))
	(if (= saveb 18)
	    (#mouse-ds))
	(if (= saveb 19)
	    (#mouse-dsc))
	(if (= saveb 20)
	    (#mouse-dl))
	(if (= saveb 21)
	    (#mouse-dlc))
	(if (= saveb 22)
	    (#mouse-dls))
	(if (= saveb 23)
	    (#mouse-dlsc))
	(if (= saveb 24)
	    (#mouse-do))
	(if (= saveb 25)
	    (#mouse-doc))
	(if (= saveb 26)
	    (#mouse-dos))
	(if (= saveb 27)
	    (#mouse-dosc))
	(if (= saveb 28)
	    (#mouse-dol))
	(if (= saveb 29)
	    (#mouse-dolc))
	(if (= saveb 30)
	    (#mouse-dols))
	(if (= saveb 31)
	    (#mouse-dolsc))
	(if (= saveb 32)
	    (#mouse-u))
	(if (= saveb 33)
	    (#mouse-uc))
	(if (= saveb 34)
	    (#mouse-us))
	(if (= saveb 35)
	    (#mouse-usc))
	(if (= saveb 36)
	    (#mouse-ul))
	(if (= saveb 37)
	    (#mouse-ulc))
	(if (= saveb 38)
	    (#mouse-uls))
	(if (= saveb 39)
	    (#mouse-ulsc))
	(if (= saveb 40)
	    (#mouse-uo))
	(if (= saveb 41)
	    (#mouse-uoc))
	(if (= saveb 42)
	    (#mouse-uos))
	(if (= saveb 43)
	    (#mouse-uosc))
	(if (= saveb 44)
	    (#mouse-uol))
	(if (= saveb 45)
	    (#mouse-uolc))
	(if (= saveb 46)
	    (#mouse-uols))
	(if (= saveb 47)
	    (#mouse-uolsc))
    )

    ; individual button bindings

    (#mouse-u			; up
     	(if (! (#mouse-click-p))
	    (progn 
		   (#mouse-set-region)
		   (Copy-region-to-kill-buffer)
	    ))
    )

    (#mouse-uc			; up/command
    )

    (#mouse-us			; up/shift
     	(if (! (#mouse-click-p))
	    (progn 
		   (#mouse-set-region)
		   (delete-to-killbuffer)
	    ))
    )

    (#mouse-usc			; up/shift/command
    )

    (#mouse-ul			; up/lock
    )

    (#mouse-ulc			; up/lock/command
    )

    (#mouse-uls			; up/lock/shift
    )

    (#mouse-ulsc		; up/lock/shift/command
    )

    (#mouse-uo			; up/option
     	(if (#mouse-click-p)
	    (yank-from-killbuffer)
	)
    )

    (#mouse-uoc			; up/option/command
    )

    (#mouse-uos			; up/option/shift
	(if (#mouse-click-p)	; click
	    (yank-buffer (get-tty-buffer "Insert contents of buffer: "))
	)
    )

    (#mouse-uosc		; up/option/shift
    )

    (#mouse-uol			; up/option/lock
    )

    (#mouse-uolc		; up/option/lock
    )

    (#mouse-uols		; up/option/lock/shift
    )

    (#mouse-uolsc		; up/option/lock/shift/command
    )
    
    (#mouse-d			; down
    )

    (#mouse-dc			; down/command
    )

    (#mouse-ds			; down/shift
    )

    (#mouse-dsc			; down/shift/command
    )

    (#mouse-dl			; down/lock
    )

    (#mouse-dlc			; down/lock/command
    )

    (#mouse-dls			; down/lock/shift
    )

    (#mouse-dlsc		; down/lock/shift/command
    )

    (#mouse-do			; down/option
    )

    (#mouse-doc			; down/option/command
    )

    (#mouse-dos			; down/option/shift
    )

    (#mouse-dosc		; down/option/shift
    )

    (#mouse-dol			; down/option/lock
    )

    (#mouse-dolc		; down/option/lock
    )

    (#mouse-dols		; down/option/lock/shift
    )

    (#mouse-dolsc		; down/option/lock/shift/command
    )

    (#mouse-set-region		; set the region to be from last dot to dot.
	(set-mark)
	(goto-character #mouse-last-dot)
	(exchange-dot-and-mark)
    )

    (#mouse-click-p clickp
     	(if (= (dot) #mouse-last-dot)
	    (setq clickp 1)
	    (setq clickp 0)
	))
    
    (#mouse-scroll-region	 ; out of range actions:
				;    left margin -- hard to generate, ignored
				;    right margin -- simulate scroll bar
				;      line 1 -- previous page
				;      line 24/25 -- next page
				;      other lines -- thumbing
				;    top margin -- previous page
				;    bottom margin -- next page
				; 
				; if shifted, deal with lines. 
				;    line 1 scrolls one line down
				;    line 24/25 scrolls one line up
				;    else line to top;  with option to bottom.
				;
				; if up stroke is in same place as down
				; stroke, don't do anything, so clicks in
				; the scroll region don't do the action
				; twice.
	(if (= down 1)
	    (if (= shift 1)
		(do-lines)
		(do-pages))
	)
	(if (& (= up 1)
	       (| (!= x #mouse-last-x) (!= y #mouse-last-y)))
	    (if (= shift 1)
		(do-lines)
		(do-pages)
	    )
	)
	(#mouse-set-region)
    )

    (do-pages			; large motions via pages and thumbing
	(if (| (= y 0) (= y 1) (= y 24) (= y 25))
	    (progn 
		   (if (| (= y 0) (= y 1))
		       (previous-page)
		       (Next-Page)
		   ))
	    (if (= x 81)
		(goto-percent (/ (* y 100) 25))
	    )
	))

    (do-lines			; fine control over lines
	(if (= x 81)
	    (if (| (= y 1) (= y 24) (= y 25))
		(if (| (= y 0) (= y 1))
		    (scroll-one-line-down)
		    (scroll-one-line-up)
		)
		(progn
		      (move-dot-to-x-y 1 y)
		      (if (= option 0)
			  (line-to-top-of-window)
			  (line-to-bottom-of-window))
		)
	    )
	)
    )

    (line-to-bottom-of-window nlines i
	(line-to-top-of-window)
	(setq i 0)
	(setq nlines (- (window-height) 1))
	(while (< i nlines)
	       (scroll-one-line-down)
	       (setq i (+ i 1))
	)
    )

    (goto-percent
       (goto-character (/ (* (buffer-size) (arg 1)) 100))
   )
)
    
(bind-to-key "move-mac-cursor" "\em")
