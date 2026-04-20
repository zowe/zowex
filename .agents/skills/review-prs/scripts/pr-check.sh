#!/bin/bash

# PR Review Automated Checks
# Usage: ./pr-check.sh [base-branch]

BASE_BRANCH=${1:-"main"}

# ANSI Color Codes
CYAN='\033[0;36m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}PR Review Automated Checks${NC}"
echo -e "${CYAN}========================================${NC}\n"

CHANGED_FILES=$(git diff --name-only "$BASE_BRANCH...HEAD" 2>/dev/null)

if [ -z "$CHANGED_FILES" ]; then
    echo -e "${YELLOW}No changes detected against $BASE_BRANCH${NC}"
    exit 0
fi

echo -e "${GREEN}Changed files:${NC}"
while read -r file; do
    echo "  - $file"
done <<< "$CHANGED_FILES"
echo ""

echo -e "${YELLOW}Checking for potential secrets...${NC}"
SECRETS_FOUND=false
SECRET_PATTERNS=("password[[:space:]]*=" "api[-_]?key[[:space:]]*=" "secret[[:space:]]*=" "token[[:space:]]*=.*['\"] [a-zA-Z0-9]{20,}['\"]")

while read -r file; do
    if [ -f "$file" ]; then
        for pattern in "${SECRET_PATTERNS[@]}"; do
            MATCHES=$(grep -nE "$pattern" "$file")
            if [ -not -z "$MATCHES" ]; then
                echo -e "  ${RED}🔴 Potential secret in $file${NC}"
                while read -r line; do
                    echo "     Line $line"
                done <<< "$MATCHES"
                SECRETS_FOUND=true
            fi
        done
    fi
done <<< "$CHANGED_FILES"

if [ "$SECRETS_FOUND" = false ]; then
    echo -e "  ${GREEN}✓ No obvious secrets detected${NC}"
fi
echo ""

if [ "$DEPRECATED_FOUND" = false ]; then
    echo -e "  ${GREEN}✓ No deprecated patterns detected${NC}"
fi
echo ""

echo -e "${YELLOW}Checking error handling patterns...${NC}"
while read -r file; do
    if [[ -f "$file" && "$file" =~ \.(ts|tsx)$ ]]; then
        if grep -qE "catch[[:space:]]*\([[:space:]]*[a-zA-Z0-9_]+[[:space:]]*\)[[:space:]]*\{[[:space:]]*\}" "$file"; then
            echo -e "  ${YELLOW}🟡 Empty catch block in $file${NC}"
        fi
        
        if grep -qE "console\.(log|error|warn)" "$file"; then
            echo -e "  ${YELLOW}🟡 console.* usage in $file - consider ZoweLogger${NC}"
        fi
    fi
done <<< "$CHANGED_FILES"
echo ""

echo -e "${YELLOW}Checking file sizes...${NC}"
while read -r file; do
    if [ -f "$file" ]; then
        LINE_COUNT=$(wc -l < "$file" | tr -d ' ')
        if [ "$LINE_COUNT" -gt 500 ]; then
            echo -e "  ${YELLOW}🟡 Large file: $file ($LINE_COUNT lines)${NC}"
        fi
    fi
done <<< "$CHANGED_FILES"
echo ""

echo -e "${CYAN}========================================${NC}"
echo -e "${CYAN}Automated checks complete${NC}"
echo -e "${CYAN}Run \"pnpm lint\" and \"pnpm test\" for full validation${NC}"
echo -e "${CYAN}========================================${NC}"