#!/bin/bash
# =============================================================================
# سكريبت رفع المشروع على GitHub
# الاستخدام: bash upload.sh
# =============================================================================

# رابط المستودع
REPO_URL="https://github.com/Rama-turkmani/CG-Lab-4.git"

echo "============================================"
echo "  Uploading: Catch the Stars! Project"
echo "  Target: $REPO_URL"
echo "============================================"

# التأكد من وجود git
if ! command -v git &> /dev/null; then
    echo "ERROR: git is not installed!"
    echo "Please install git first: https://git-scm.com/downloads"
    exit 1
fi

# الانتقال لمجلد المشروع (مجلد السكريبت)
cd "$(dirname "$0")" || exit 1

echo ""
echo "[1/6] Initializing git repository..."
git init

echo ""
echo "[2/6] Adding remote origin..."
# إزالة remote قديم إن وجد
git remote remove origin 2>/dev/null
git remote add origin "$REPO_URL"

echo ""
echo "[3/6] Creating .gitignore..."
cat > .gitignore << 'EOF'
# Visual Studio files
.vs/
x64/
Debug/
Release/
*.user
*.suo
*.sdf
*.opensdf
*.db
*.opendb
*.ipch

# Build output
*.exe
*.obj
*.pdb
*.ilk
*.log
*.tlog
*.idb
*.pch

# OS files
Thumbs.db
.DS_Store
desktop.ini

# PDF books (too large for git)
*.pdf
EOF

echo ""
echo "[4/6] Staging all files..."
git add -A

echo ""
echo "[5/6] Creating commit..."
git commit -m "Catch the Stars! - Computer Graphics Project

- Simple 2D game using OpenGL 3.3 (GLEW + GLFW)
- Concepts: VBO, EBO, VAO, Shaders, Uniforms
- Player catches falling stars with a paddle
- Score system, lives, increasing difficulty
- Visual effects: twinkling stars, pulsing colors
- Includes: HTML presentation + code description"

echo ""
echo "[6/6] Pushing to GitHub..."
git branch -M main
git push -u origin main --force

echo ""
echo "============================================"
echo "  Upload complete!"
echo "  Visit: https://github.com/Rama-turkmani/CG-Lab-4"
echo "============================================"
