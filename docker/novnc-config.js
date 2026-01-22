// noVNC Configuration for Enhanced Mobile and Clipboard Support
// This configuration enables clipboard sharing and improves mobile keyboard input

(function() {
    'use strict';

    // Wait for noVNC UI to be ready
    window.addEventListener('load', function() {
        if (typeof UI !== 'undefined' && UI.rfb) {
            // Enable clipboard integration
            if (UI.rfb.clipboardUp) {
                UI.rfb.clipboardUp = true;
            }
            
            // Configure for mobile devices
            const isMobile = /Android|webOS|iPhone|iPad|iPod|BlackBerry|IEMobile|Opera Mini/i.test(navigator.userAgent);
            
            if (isMobile) {
                console.log('Mobile device detected, enabling mobile optimizations');
                
                // Enable virtual keyboard button
                const keyboardButton = document.getElementById('noVNC_keyboard_button');
                if (keyboardButton) {
                    keyboardButton.style.display = 'inline';
                }
                
                // Enable touch gestures
                if (UI.rfb && UI.rfb._gesture) {
                    UI.rfb._gesture = true;
                }
                
                // Set scaling mode to remote for better mobile experience
                if (typeof UI.setViewClip === 'function') {
                    UI.setViewClip(false);
                }
                if (typeof UI.setScaling === 'function') {
                    UI.setScaling(true);
                }
            }
            
            // Enable clipboard events
            if (UI.rfb && UI.rfb._canvas) {
                UI.rfb._canvas.addEventListener('paste', function(e) {
                    e.preventDefault();
                    const text = e.clipboardData.getData('text/plain');
                    if (text && UI.rfb.clipboardPasteFrom) {
                        UI.rfb.clipboardPasteFrom(text);
                    }
                });
                
                UI.rfb._canvas.addEventListener('copy', function(e) {
                    if (UI.clipboardText) {
                        e.clipboardData.setData('text/plain', UI.clipboardText);
                        e.preventDefault();
                    }
                });
            }
            
            // Add mobile keyboard input helper
            if (isMobile) {
                createMobileKeyboardHelper();
            }
        }
    });
    
    // Create a mobile-friendly keyboard input helper
    function createMobileKeyboardHelper() {
        const container = document.getElementById('noVNC_container') || document.body;
        
        // Create floating keyboard button
        const keyboardBtn = document.createElement('button');
        keyboardBtn.id = 'mobile-keyboard-btn';
        keyboardBtn.innerHTML = '⌨️';
        keyboardBtn.style.cssText = `
            position: fixed;
            bottom: 20px;
            right: 20px;
            width: 50px;
            height: 50px;
            border-radius: 25px;
            background: rgba(0, 0, 0, 0.7);
            color: white;
            border: 2px solid #fff;
            font-size: 24px;
            z-index: 10000;
            cursor: pointer;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.3);
        `;
        
        // Create hidden input for mobile keyboard
        const hiddenInput = document.createElement('input');
        hiddenInput.id = 'mobile-keyboard-input';
        hiddenInput.type = 'text';
        hiddenInput.style.cssText = `
            position: fixed;
            top: -100px;
            left: -100px;
            opacity: 0;
            pointer-events: none;
        `;
        
        container.appendChild(keyboardBtn);
        container.appendChild(hiddenInput);
        
        // Handle keyboard button click
        keyboardBtn.addEventListener('click', function(e) {
            e.preventDefault();
            e.stopPropagation();
            hiddenInput.focus();
            
            // Show visual feedback
            keyboardBtn.style.background = 'rgba(0, 120, 215, 0.8)';
            setTimeout(() => {
                keyboardBtn.style.background = 'rgba(0, 0, 0, 0.7)';
            }, 200);
        });
        
        // Forward input to VNC
        hiddenInput.addEventListener('input', function(e) {
            const text = this.value;
            if (text && typeof UI !== 'undefined' && UI.rfb) {
                // Send each character
                for (let char of text) {
                    const keysym = char.charCodeAt(0);
                    if (UI.rfb.sendKey) {
                        UI.rfb.sendKey(keysym, null, true);
                        UI.rfb.sendKey(keysym, null, false);
                    }
                }
                this.value = '';
            }
        });
        
        // Handle special keys
        hiddenInput.addEventListener('keydown', function(e) {
            if (typeof UI !== 'undefined' && UI.rfb && UI.rfb.sendKey) {
                let keysym = null;
                
                switch(e.key) {
                    case 'Enter':
                        keysym = 0xFF0D; // XK_Return
                        break;
                    case 'Backspace':
                        keysym = 0xFF08; // XK_BackSpace
                        break;
                    case 'Tab':
                        keysym = 0xFF09; // XK_Tab
                        e.preventDefault();
                        break;
                    case 'Escape':
                        keysym = 0xFF1B; // XK_Escape
                        e.preventDefault();
                        break;
                    case 'Delete':
                        keysym = 0xFFFF; // XK_Delete
                        break;
                    case 'ArrowUp':
                        keysym = 0xFF52; // XK_Up
                        e.preventDefault();
                        break;
                    case 'ArrowDown':
                        keysym = 0xFF54; // XK_Down
                        e.preventDefault();
                        break;
                    case 'ArrowLeft':
                        keysym = 0xFF51; // XK_Left
                        e.preventDefault();
                        break;
                    case 'ArrowRight':
                        keysym = 0xFF53; // XK_Right
                        e.preventDefault();
                        break;
                }
                
                if (keysym !== null) {
                    UI.rfb.sendKey(keysym, null, true);
                    UI.rfb.sendKey(keysym, null, false);
                }
            }
        });
        
        console.log('Mobile keyboard helper initialized');
    }
    
    // Store clipboard text for copy operations
    if (typeof UI !== 'undefined') {
        const originalClipboardReceive = UI.clipboardReceive;
        if (originalClipboardReceive) {
            UI.clipboardReceive = function(rfb, text) {
                UI.clipboardText = text;
                
                // Try to write to system clipboard
                if (navigator.clipboard && navigator.clipboard.writeText) {
                    navigator.clipboard.writeText(text).catch(function(err) {
                        console.log('Could not copy to clipboard:', err);
                    });
                }
                
                // Show notification
                console.log('Clipboard updated:', text.substring(0, 50) + (text.length > 50 ? '...' : ''));
                
                if (originalClipboardReceive) {
                    originalClipboardReceive.call(this, rfb, text);
                }
            };
        }
    }
})();
