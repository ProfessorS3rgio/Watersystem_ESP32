<?php

namespace App\Http\Controllers;
use App\Models\Reading;
use App\Models\Bill;
use App\Models\Setting;
use App\Models\Customer;
use App\Models\BillTransaction;
use Carbon\Carbon;
use Illuminate\Http\Request;
use PhpOffice\PhpSpreadsheet\Spreadsheet;
use PhpOffice\PhpSpreadsheet\Writer\Xlsx;
use PhpOffice\PhpSpreadsheet\Style\Alignment;
use PhpOffice\PhpSpreadsheet\Style\Border;
use PhpOffice\PhpSpreadsheet\Style\Fill;
use PhpOffice\PhpSpreadsheet\Style\Font;
use PhpOffice\PhpSpreadsheet\Style\NumberFormat;
use PhpOffice\PhpSpreadsheet\Style\Color;


class BillController extends Controller
{
    public function index(Request $request)
    {
        $status = $request->query('status');
        $customerId = $request->query('customer_id');
        $customerSearch = $request->query('customer_search');

        $query = Bill::with('customer');

        if ($status) {
            $query->where('status', $status);
        }

        if ($customerId) {
            $query->where('customer_id', $customerId);
        }

        if ($customerSearch) {
            $query->whereHas('customer', function ($q) use ($customerSearch) {
                $q->where('customer_name', 'like', '%' . $customerSearch . '%')
                  ->orWhere('account_no', 'like', '%' . $customerSearch . '%');
            });
        }

        $bills = $query->orderBy('created_at', 'desc')->paginate(20);

        return response()->json([
            'data' => $bills,
        ]);
    }

public function monthlyReport(Request $request)
{
     $validated = $request->validate([
        'month' => ['required', 'date_format:Y-m'],
        'brgy_id' => ['nullable', 'integer', 'exists:barangay_sequence,brgy_id'],
    ]);

    $month = Carbon::createFromFormat('Y-m', $validated['month'])->startOfMonth();
    $barangayId = $validated['brgy_id'] ?? null;
    
    // Define the billing cycle: include first 5 days of next month as part of this month
    $billingStart = $month->copy()->startOfMonth();
    $billingEnd = $month->copy()->endOfMonth()->addDays(5); // Include up to 5th of next month

    if (!$barangayId) {
        return $this->allBarangaysMonthlyReport($month, $billingStart, $billingEnd);
    }
    
    $billsQuery = Bill::with(['customer', 'reading'])
        ->whereBetween('bill_date', [$billingStart, $billingEnd])
        ->orderBy('customer_account_number');

    if ($barangayId) {
        $billsQuery->whereHas('customer', fn ($customer) => $customer->where('brgy_id', $barangayId));
    }

    $bills = $billsQuery->get();
    
    $customersQuery = Customer::orderBy('account_no');
    if ($barangayId) {
        $customersQuery->where('brgy_id', $barangayId);
    }
    $allCustomers = $customersQuery->get()->keyBy('account_no');
    
    $reportData = [];
    $processedAccounts = [];
    
    foreach ($bills as $bill) {
        $accountNo = $bill->customer_account_number;
        $processedAccounts[] = $accountNo;
        $reportData[] = [
            'account_no' => $accountNo,
            'customer_name' => $bill->customer->customer_name ?? 'Unknown',
            'previous_reading' => $bill->reading->previous_reading ?? '',
            'current_reading' => $bill->reading->current_reading ?? '',
            'usage_m3' => $bill->reading->usage_m3 ?? '',
            'status' => $bill->status,
            'total_due' => (float) $bill->total_due,
            'has_bill' => true,
        ];
    }
    
    foreach ($allCustomers as $accountNo => $customer) {
        if (!in_array($accountNo, $processedAccounts)) {
            $reportData[] = [
                'account_no' => $customer->account_no,
                'customer_name' => $customer->customer_name,
                'previous_reading' => '',
                'current_reading' => '',
                'usage_m3' => '',
                'status' => $customer->status == 'disconnected' ? 'Disconnected' : 'No Bill',
                'total_due' => 0,
                'has_bill' => false,
            ];
        }
    }
    
    usort($reportData, function($a, $b) {
        return strcmp($a['account_no'], $b['account_no']);
    });

    $barangay = $barangayId
        ? \App\Models\BarangaySequence::find($barangayId)?->barangay
        : 'All Barangays';
    $filenameBarangay = preg_replace('/[^A-Za-z0-9_-]+/', '-', $barangay);
    $filename = "monthly-billing-report-{$filenameBarangay}-{$month->format('Y-m')}.xlsx";

    $spreadsheet = new Spreadsheet();
    $sheet = $spreadsheet->getActiveSheet();
    $sheet->setTitle("Billing {$month->format('M Y')}");
    
    $sheet->getColumnDimension('A')->setWidth(18);
    $sheet->getColumnDimension('B')->setWidth(35);
    $sheet->getColumnDimension('C')->setWidth(15);
    $sheet->getColumnDimension('D')->setWidth(15);
    $sheet->getColumnDimension('E')->setWidth(12);
    $sheet->getColumnDimension('F')->setWidth(18);
    $sheet->getColumnDimension('G')->setWidth(18);
    
    // TITLE
    $sheet->setCellValue('A1', 'MONTHLY WATER BILLING REPORT');
    $sheet->mergeCells('A1:G1');
    $sheet->getStyle('A1:G1')->applyFromArray([
        'font' => ['bold' => true, 'size' => 16, 'color' => ['rgb' => 'FFFFFF']],
        'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => '0F3D5E']],
        'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER, 'vertical' => Alignment::VERTICAL_CENTER],
    ]);
    $sheet->getRowDimension(1)->setRowHeight(35);
    
    // SUBTITLE
    $sheet->setCellValue('A2', "Barangay: {$barangay}  •  Billing Month: {$month->format('F Y')}");
    $sheet->mergeCells('A2:G2');
    $sheet->getStyle('A2:G2')->applyFromArray([
        'font' => ['italic' => true, 'size' => 11, 'color' => ['rgb' => '475569']],
        'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER, 'vertical' => Alignment::VERTICAL_CENTER],
    ]);
    $sheet->getRowDimension(2)->setRowHeight(22);
    
    // HEADERS
    $headers = ['Account Number', 'Customer Name', 'Previous Reading', 'Present Reading', 'Usage (m³)', 'Status', 'Total Amount'];
    $col = 'A';
    foreach ($headers as $header) {
        $sheet->setCellValue($col . '4', $header);
        $col++;
    }
    $sheet->getStyle('A4:G4')->applyFromArray([
        'font' => ['bold' => true, 'size' => 11, 'color' => ['rgb' => 'FFFFFF']],
        'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => '0E7490']],
        'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER, 'vertical' => Alignment::VERTICAL_CENTER, 'wrapText' => true],
        'borders' => ['allBorders' => ['borderStyle' => Border::BORDER_THIN, 'color' => ['rgb' => '0891B2']]],
    ]);
    $sheet->getRowDimension(4)->setRowHeight(25);
    
    // DATA ROWS
    $row = 5;
    $totalAmount = 0;
    $totalPaid = 0;
    $totalPending = 0;
    
    foreach ($reportData as $index => $data) {
        $sheet->setCellValue('A' . $row, $data['account_no']);
        $sheet->setCellValue('B' . $row, $data['customer_name']);
        $sheet->setCellValue('C' . $row, $data['previous_reading'] !== '' ? $data['previous_reading'] : '');
        $sheet->setCellValue('D' . $row, $data['current_reading'] !== '' ? $data['current_reading'] : '');
        $sheet->setCellValue('E' . $row, $data['usage_m3'] !== '' ? $data['usage_m3'] : '');
        
        $statusValue = $data['status'] === 'Disconnected' ? 'Disconnected' : ucfirst(strtolower($data['status']));
        $sheet->setCellValue('F' . $row, $statusValue);
        
        switch (strtolower($data['status'])) {
            case 'paid': $statusColor = '16A34A'; break;
            case 'pending': $statusColor = 'EA580C'; break;
            case 'disconnected': $statusColor = 'DC2626'; break;
            case 'void': $statusColor = '6B7280'; break;
            case 'due': $statusColor = 'CA8A04'; break;
            default: $statusColor = '4B5563';
        }
        
        $sheet->getStyle('F' . $row)->applyFromArray([
            'font' => ['bold' => true, 'color' => ['rgb' => $statusColor]],
            'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER],
        ]);
        
        $sheet->setCellValue('G' . $row, $data['total_due']);
        $sheet->getStyle('G' . $row)->getNumberFormat()->setFormatCode('₱#,##0.00');
        $sheet->getStyle('G' . $row)->getAlignment()->setHorizontal(Alignment::HORIZONTAL_RIGHT);
        
        $sheet->getStyle('A' . $row . ':G' . $row)->applyFromArray([
            'borders' => ['allBorders' => ['borderStyle' => Border::BORDER_THIN, 'color' => ['rgb' => 'D1D5DB']]],
        ]);
        
        // Alternate row colors - FIXED
        if ($index % 2 == 1) {
            $sheet->getStyle('A' . $row . ':G' . $row)->getFill()
                ->setFillType(Fill::FILL_SOLID)
                ->setStartColor(new Color('F8FAFC'));
        }
        
        $sheet->getStyle('C' . $row . ':E' . $row)->getAlignment()
            ->setHorizontal(Alignment::HORIZONTAL_CENTER);
        
        if ($data['has_bill']) {
            $totalAmount += $data['total_due'];
            if (strtolower($data['status']) == 'paid') $totalPaid += $data['total_due'];
            if (strtolower($data['status']) == 'pending') $totalPending += $data['total_due'];
        }
        
        $row++;
    }
    
    // SUMMARY
    $row++;
    $sheet->setCellValue('A' . $row, 'SUMMARY');
    $sheet->mergeCells('A' . $row . ':G' . $row);
    $sheet->getStyle('A' . $row . ':G' . $row)->applyFromArray([
        'font' => ['bold' => true, 'size' => 12, 'color' => ['rgb' => 'FFFFFF']],
        'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => '0F3D5E']],
        'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER],
    ]);
    $row++;
    
    $summaryData = [
        ['Total Customers', count($reportData)],
        ['Active with Bills', count(array_filter($reportData, fn($d) => $d['has_bill']))],
        ['Disconnected', count(array_filter($reportData, fn($d) => $d['status'] === 'Disconnected'))],
        ['No Bill', count(array_filter($reportData, fn($d) => $d['status'] === 'No Bill'))],
        ['', ''],
        ['Total Amount Due', '₱' . number_format($totalAmount, 2)],
        ['Total Collected', '₱' . number_format($totalPaid, 2)],
        ['Total Pending', '₱' . number_format($totalPending, 2)],
    ];
    
    foreach ($summaryData as $summaryRow) {
        $sheet->setCellValue('A' . $row, $summaryRow[0]);
        $sheet->setCellValue('B' . $row, $summaryRow[1]);
        $sheet->mergeCells('B' . $row . ':C' . $row);
        
        if (!empty($summaryRow[0])) {
            $sheet->getStyle('A' . $row)->getFont()->setBold(true);
        }
        if (strpos($summaryRow[0], 'Total') === 0) {
            $sheet->getStyle('A' . $row . ':C' . $row)->getFont()
                ->setBold(true)->setSize(11);
        }
        $row++;
    }
    
    // FOOTER
    $row++;
    $sheet->setCellValue('A' . $row, 'Generated on: ' . now()->format('F d, Y  h:i A'));
    $sheet->mergeCells('A' . $row . ':G' . $row);
    $sheet->getStyle('A' . $row)->applyFromArray([
        'font' => ['italic' => true, 'size' => 9, 'color' => ['rgb' => '6B7280']],
        'alignment' => ['horizontal' => Alignment::HORIZONTAL_RIGHT],
    ]);
    
    $sheet->freezePane('A5');
    $sheet->setAutoFilter('A4:G4');
    
    $writer = new Xlsx($spreadsheet);
    $temporaryFile = tempnam(sys_get_temp_dir(), 'monthly-billing-report-') . '.xlsx';
    $writer->save($temporaryFile);

    return response()->download($temporaryFile, $filename, [
        'Content-Type' => 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
    ])->deleteFileAfterSend(true);
}

    private function allBarangaysMonthlyReport(Carbon $month, Carbon $billingStart, Carbon $billingEnd)
    {
        $barangays = \App\Models\BarangaySequence::orderBy('brgy_id')->get();
        $customerTypes = \App\Models\CustomerType::pluck('type_name', 'type_id');
        $customersByBarangay = Customer::orderBy('account_no')->get()->groupBy('brgy_id');
        $billsByAccount = Bill::with(['customer', 'reading'])
            ->whereBetween('bill_date', [$billingStart, $billingEnd])
            ->orderBy('bill_date', 'desc')
            ->get()
            ->keyBy('customer_account_number');

        $spreadsheet = new Spreadsheet();
        $spreadsheet->getDefaultStyle()->getFont()->setName('Calibri')->setSize(11);
        $moneyFormat = '"PHP "#,##0.00';
        $colors = [
            'navy' => '16324F', 'teal' => '0F766E', 'tealLight' => 'CCFBF1',
            'gold' => 'D9A441', 'goldLight' => 'FEF3C7', 'slate' => '475569',
            'muted' => '64748B', 'line' => 'CBD5E1', 'soft' => 'F1F5F9',
            'white' => 'FFFFFF', 'danger' => 'B91C1C',
        ];

        $sectionForType = function (?string $typeName): string {
            $type = strtolower((string) $typeName);
            if (str_contains($type, 'communal')) return 'LEVEL - II';
            if (str_contains($type, 'commercial')) return 'COMMERCIAL';
            if (str_contains($type, 'officer')) return "OFFICER'S RATE";
            return 'LEVEL - III';
        };

        $summary = [];

        foreach ($barangays as $sheetIndex => $barangay) {
            $sheet = $sheetIndex === 0 ? $spreadsheet->getActiveSheet() : $spreadsheet->createSheet();
            $sheetName = substr(preg_replace('/[\\\\\\/\\?\\*\\[\\]:]+/', '-', $barangay->barangay), 0, 31);
            $sheet->setTitle($sheetName ?: "Barangay {$barangay->brgy_id}");
            $sheet->getDefaultRowDimension()->setRowHeight(20);
            $sheet->setShowGridlines(false);
            $sheet->getSheetView()->setZoomScale(90);
            $sheet->getTabColor()->setRGB($colors['teal']);

            foreach (['A' => 12, 'B' => 8, 'C' => 30, 'D' => 17, 'E' => 18, 'F' => 17, 'G' => 20, 'H' => 15] as $column => $width) {
                $sheet->getColumnDimension($column)->setWidth($width);
            }

            $sheet->mergeCells('A1:H1');
            $sheet->setCellValue('A1', 'DMBC WATER BILLING REPORT');
            $sheet->mergeCells('A2:H2');
            $sheet->setCellValue('A2', strtoupper($barangay->barangay));
            $sheet->mergeCells('A3:H3');
            $sheet->setCellValue('A3', 'Billing period: ' . $billingStart->format('M d, Y') . ' - ' . $billingEnd->format('M d, Y') . '  |  Report month: ' . $month->format('F Y'));
            $sheet->getRowDimension(1)->setRowHeight(32);
            $sheet->getRowDimension(2)->setRowHeight(26);
            $sheet->getStyle('A1:H1')->applyFromArray([
                'font' => ['bold' => true, 'size' => 18, 'color' => ['rgb' => $colors['white']]],
                'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['navy']]],
                'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER, 'vertical' => Alignment::VERTICAL_CENTER],
            ]);
            $sheet->getStyle('A2:H2')->applyFromArray([
                'font' => ['bold' => true, 'size' => 13, 'color' => ['rgb' => $colors['white']]],
                'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['teal']]],
                'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER, 'vertical' => Alignment::VERTICAL_CENTER],
            ]);
            $sheet->getStyle('A3:H3')->applyFromArray([
                'font' => ['italic' => true, 'size' => 10, 'color' => ['rgb' => $colors['slate']]],
                'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['soft']]],
                'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER, 'vertical' => Alignment::VERTICAL_CENTER],
            ]);

            $row = 3;
            $barangaySummary = [
                'barangay' => $barangay->barangay,
                'LEVEL - II' => 0,
                'LEVEL - III' => 0,
                'COMMERCIAL' => 0,
                "OFFICER'S RATE" => 0,
                'gross' => 0,
                'delayed' => 0,
                'penalty' => 0,
            ];

            $customers = $customersByBarangay->get($barangay->brgy_id, collect())
                ->groupBy(fn ($customer) => $sectionForType($customerTypes[$customer->type_id] ?? null));

            foreach (['LEVEL - III', 'LEVEL - II', 'COMMERCIAL', "OFFICER'S RATE"] as $section) {
                $sectionCustomers = $customers->get($section, collect());
                if ($sectionCustomers->isEmpty()) {
                    continue;
                }

                $row++;
                $sheet->mergeCells("A{$row}:H{$row}");
                $sheet->setCellValue("A{$row}", $section);
                $sheet->getRowDimension($row)->setRowHeight(24);
                $sheet->getStyle("A{$row}:H{$row}")->applyFromArray([
                    'font' => ['bold' => true, 'size' => 11, 'color' => ['rgb' => $colors['white']]],
                    'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['teal']]],
                    'alignment' => ['horizontal' => Alignment::HORIZONTAL_LEFT, 'vertical' => Alignment::VERTICAL_CENTER, 'indent' => 1],
                ]);

                $row++;
                $sheet->mergeCells("C{$row}:D{$row}");
                $sheet->mergeCells("F{$row}:G{$row}");
                $sheet->fromArray([
                    '',
                    '',
                    "Billing Month: {$billingStart->format('M. j')}-{$month->copy()->endOfMonth()->format('j, Y')}",
                    '',
                    'Collection Date:',
                    'To be collect by the month of:',
                    '',
                    $billingEnd->copy()->addMonth()->format('M. Y'),
                ], null, "A{$row}");
                $sheet->getStyle("A{$row}:H{$row}")->applyFromArray([
                    'font' => ['size' => 9, 'color' => ['rgb' => $colors['muted']]],
                    'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['soft']]],
                    'alignment' => ['vertical' => Alignment::VERTICAL_CENTER],
                ]);

                $row++;
                $sheet->fromArray(['USAGE (m3)', 'NO.', 'CUSTOMER NAME', 'PREVIOUS PENALTY', 'MONTHLY COLLECTION', 'ACCOUNT / METER NO.', 'OUTSTANDING BALANCE', 'PENALTY'], null, "A{$row}");
                $sheet->getRowDimension($row)->setRowHeight(34);
                $sheet->getStyle("A{$row}:H{$row}")->applyFromArray([
                    'font' => ['bold' => true, 'size' => 9, 'color' => ['rgb' => $colors['white']]],
                    'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['navy']]],
                    'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER, 'vertical' => Alignment::VERTICAL_CENTER, 'wrapText' => true],
                    'borders' => ['bottom' => ['borderStyle' => Border::BORDER_MEDIUM, 'color' => ['rgb' => $colors['gold']]]],
                ]);

                $row++;
                $sectionStartRow = $row;
                $sectionUsage = 0;
                $sectionCollection = 0;
                $sectionCollectable = 0;
                $sectionPenalty = 0;
                $number = 1;

                foreach ($sectionCustomers as $customer) {
                    $bill = $billsByAccount->get($customer->account_no);
                    $status = strtolower((string) ($bill?->status ?? ''));
                    $usage = $bill?->reading?->usage_m3;
                    $penalty = (float) ($bill?->penalty ?? 0);
                    $amount = (float) ($bill?->total_due ?? 0);
                    $monthlyCollection = $bill && $status === 'paid' ? $amount : null;
                    $collectable = $bill && in_array($status, ['pending', 'due'], true) ? $amount : null;
                    $displayName = $customer->customer_name;

                    if (!$bill && $customer->status === 'disconnected') {
                        $displayName .= ' CLOSED';
                    }

                    $sheet->fromArray([
                        $usage ?? '',
                        $number,
                        $displayName,
                        '',
                        $monthlyCollection,
                        $customer->account_no,
                        $collectable,
                        $penalty > 0 ? $penalty : null,
                    ], null, "A{$row}");
                    $sheet->getStyle("A{$row}:H{$row}")->applyFromArray([
                        'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $number % 2 === 0 ? $colors['soft'] : $colors['white']]],
                        'borders' => ['bottom' => ['borderStyle' => Border::BORDER_HAIR, 'color' => ['rgb' => $colors['line']]]],
                        'alignment' => ['vertical' => Alignment::VERTICAL_CENTER],
                    ]);
                    $sheet->getStyle("A{$row}:B{$row}")->getAlignment()->setHorizontal(Alignment::HORIZONTAL_CENTER);
                    $sheet->getStyle("F{$row}")->getAlignment()->setHorizontal(Alignment::HORIZONTAL_CENTER);
                    $sheet->getStyle("D{$row}:E{$row}")->getAlignment()->setHorizontal(Alignment::HORIZONTAL_RIGHT);
                    $sheet->getStyle("G{$row}:H{$row}")->getAlignment()->setHorizontal(Alignment::HORIZONTAL_RIGHT);
                    $sheet->getStyle("D{$row}:E{$row}")->getNumberFormat()->setFormatCode($moneyFormat);
                    $sheet->getStyle("G{$row}:H{$row}")->getNumberFormat()->setFormatCode($moneyFormat);
                    if (str_ends_with($displayName, ' CLOSED')) {
                        $sheet->getStyle("C{$row}")->getFont()->setColor(new Color($colors['danger']))->setItalic(true);
                    }

                    $sectionUsage += (int) ($usage ?? 0);
                    $sectionCollection += (float) ($monthlyCollection ?? 0);
                    $sectionCollectable += (float) ($collectable ?? 0);
                    $sectionPenalty += $penalty;
                    $barangaySummary[$section] += (float) ($monthlyCollection ?? 0);
                    $barangaySummary['gross'] += $amount;
                    $barangaySummary['delayed'] += (float) ($collectable ?? 0);
                    $barangaySummary['penalty'] += $penalty;
                    $number++;
                    $row++;
                }

                $sheet->fromArray([$sectionUsage, 'TOTAL', '', '', $sectionCollection, 'TOTAL', $sectionCollectable, $sectionPenalty], null, "A{$row}");
                $sheet->getStyle("A{$row}:H{$row}")->applyFromArray([
                    'font' => ['bold' => true, 'color' => ['rgb' => $colors['navy']]],
                    'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['goldLight']]],
                    'borders' => [
                        'top' => ['borderStyle' => Border::BORDER_MEDIUM, 'color' => ['rgb' => $colors['gold']]],
                        'bottom' => ['borderStyle' => Border::BORDER_THIN, 'color' => ['rgb' => $colors['gold']]],
                    ],
                ]);
                $sheet->getStyle("D{$sectionStartRow}:E{$row}")->getNumberFormat()->setFormatCode($moneyFormat);
                $sheet->getStyle("G{$sectionStartRow}:H{$row}")->getNumberFormat()->setFormatCode($moneyFormat);
                $row++;
            }

            if ($row === 3) {
                $sheet->setCellValue('A5', 'No customers found for this barangay.');
            }

            $sheet->freezePane('A6');
            $sheet->getPageSetup()
                ->setOrientation(\PhpOffice\PhpSpreadsheet\Worksheet\PageSetup::ORIENTATION_LANDSCAPE)
                ->setPaperSize(\PhpOffice\PhpSpreadsheet\Worksheet\PageSetup::PAPERSIZE_A4)
                ->setFitToWidth(1)
                ->setFitToHeight(0);
            $sheet->getPageSetup()->setRowsToRepeatAtTopByStartAndEnd(1, 3);
            $sheet->getPageSetup()->setPrintArea("A1:H{$row}");
            $sheet->getPageMargins()->setTop(0.5)->setRight(0.3)->setLeft(0.3)->setBottom(0.5);
            $sheet->getHeaderFooter()->setOddFooter('&LDMBC Water System  |  ' . $sheet->getTitle() . '&RPage &P of &N');
            $summary[] = $barangaySummary;
        }

        $summarySheet = $spreadsheet->createSheet();
        $summarySheet->setTitle('DMBC-Summary');
        $summarySheet->setShowGridlines(false);
        $summarySheet->getSheetView()->setZoomScale(95);
        $summarySheet->getTabColor()->setRGB($colors['gold']);
        foreach (['A' => 22, 'B' => 16, 'C' => 16, 'D' => 16, 'E' => 16, 'F' => 20, 'G' => 20, 'H' => 16] as $column => $width) {
            $summarySheet->getColumnDimension($column)->setWidth($width);
        }
        $summarySheet->setCellValue('A1', 'DMBC MONTHLY COLLECTION SUMMARY');
        $summarySheet->mergeCells('A1:H1');
        $summarySheet->getStyle('A1:H1')->applyFromArray([
            'font' => ['bold' => true, 'size' => 18, 'color' => ['rgb' => $colors['white']]],
            'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['navy']]],
            'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER, 'vertical' => Alignment::VERTICAL_CENTER],
        ]);
        $summarySheet->getRowDimension(1)->setRowHeight(34);
        $summarySheet->mergeCells('A2:H2');
        $summarySheet->setCellValue('A2', $month->format('F Y') . '  |  All Barangays');
        $summarySheet->getStyle('A2:H2')->applyFromArray([
            'font' => ['bold' => true, 'size' => 12, 'color' => ['rgb' => $colors['white']]],
            'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['teal']]],
            'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER],
        ]);
        $summarySheet->fromArray(['BARANGAY', 'LEVEL II', 'LEVEL III', 'COMMERCIAL', 'OFFICERS', 'GROSS BILLED', 'OUTSTANDING', 'PENALTY'], null, 'A4');
        $summarySheet->getRowDimension(4)->setRowHeight(32);
        $summarySheet->getStyle('A4:H4')->applyFromArray([
            'font' => ['bold' => true, 'size' => 9, 'color' => ['rgb' => $colors['white']]],
            'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['navy']]],
            'alignment' => ['horizontal' => Alignment::HORIZONTAL_CENTER, 'vertical' => Alignment::VERTICAL_CENTER, 'wrapText' => true],
            'borders' => ['bottom' => ['borderStyle' => Border::BORDER_MEDIUM, 'color' => ['rgb' => $colors['gold']]]],
        ]);

        $row = 5;
        $totals = ['LEVEL - II' => 0, 'LEVEL - III' => 0, 'COMMERCIAL' => 0, "OFFICER'S RATE" => 0, 'gross' => 0, 'delayed' => 0, 'penalty' => 0];
        foreach ($summary as $item) {
            $summarySheet->fromArray([
                strtoupper($item['barangay']),
                $item['LEVEL - II'],
                $item['LEVEL - III'],
                $item['COMMERCIAL'],
                $item["OFFICER'S RATE"],
                $item['gross'],
                $item['delayed'],
                $item['penalty'],
            ], null, "A{$row}");
            $summarySheet->getStyle("A{$row}:H{$row}")->applyFromArray([
                'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $row % 2 === 0 ? $colors['white'] : $colors['soft']]],
                'borders' => ['bottom' => ['borderStyle' => Border::BORDER_HAIR, 'color' => ['rgb' => $colors['line']]]],
            ]);
            foreach ($totals as $key => $value) {
                $totals[$key] += $item[$key];
            }
            $row++;
        }
        $summarySheet->fromArray(['TOTAL', $totals['LEVEL - II'], $totals['LEVEL - III'], $totals['COMMERCIAL'], $totals["OFFICER'S RATE"], $totals['gross'], $totals['delayed'], $totals['penalty']], null, "A{$row}");
        $summarySheet->getStyle("A{$row}:H{$row}")->applyFromArray([
            'font' => ['bold' => true, 'color' => ['rgb' => $colors['navy']]],
            'fill' => ['fillType' => Fill::FILL_SOLID, 'startColor' => ['rgb' => $colors['goldLight']]],
            'borders' => [
                'top' => ['borderStyle' => Border::BORDER_MEDIUM, 'color' => ['rgb' => $colors['gold']]],
                'bottom' => ['borderStyle' => Border::BORDER_THIN, 'color' => ['rgb' => $colors['gold']]],
            ],
        ]);
        $summarySheet->getStyle("B5:H{$row}")->getNumberFormat()->setFormatCode($moneyFormat);
        $summarySheet->freezePane('A5');
        $summarySheet->setAutoFilter("A4:H{$row}");
        $summarySheet->getPageSetup()
            ->setOrientation(\PhpOffice\PhpSpreadsheet\Worksheet\PageSetup::ORIENTATION_LANDSCAPE)
            ->setPaperSize(\PhpOffice\PhpSpreadsheet\Worksheet\PageSetup::PAPERSIZE_A4)
            ->setFitToWidth(1)
            ->setFitToHeight(1);
        $summarySheet->getPageSetup()->setPrintArea("A1:H{$row}");
        $summarySheet->getHeaderFooter()->setOddFooter('&LDMBC Water System&RPage &P of &N');
        $spreadsheet->setActiveSheetIndex($spreadsheet->getIndex($summarySheet));

        $filename = "monthly-billing-report-All-Barangays-{$month->format('Y-m')}.xlsx";
        $writer = new Xlsx($spreadsheet);
        $temporaryFile = tempnam(sys_get_temp_dir(), 'monthly-billing-report-all-') . '.xlsx';
        $writer->save($temporaryFile);

        return response()->download($temporaryFile, $filename, [
            'Content-Type' => 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
        ])->deleteFileAfterSend(true);
    }

    public function indexByCustomer(Request $request, Customer $customer)
    {
        $status = $request->query('status');

        $customerId = $customer->getKey();

        $query = Bill::where('customer_id', $customerId);

        if ($status) {
            $query->where('status', $status);
        }

        $bills = $query->orderBy('created_at', 'desc')->get();

        return response()->json([
            'data' => $bills,
        ]);
    }

    public function paymentsByCustomer(Request $request, Customer $customer)
    {
        $limit = $request->query('limit', 10);

        $customerId = $customer->getKey();

        $payments = BillTransaction::whereHas('bill', function ($query) use ($customerId) {
            $query->where('customer_id', $customerId);
        })
        ->whereIn('type', ['payment', 'void'])
        ->with(['bill', 'processedBy'])
        ->orderBy('transaction_date', 'desc')
        ->limit($limit)
        ->get()
        ->map(function ($transaction) {
            return [
                'id' => $transaction->getKey(),
                'reference_number' => $transaction->bill?->reference_number ?? $transaction->bill_reference_number,
                'amount_paid' => $transaction->amount,
                'cash_received' => $transaction->cash_received,
                'change' => $transaction->change,
                'payment_method' => $transaction->payment_method,
                'processed_by' => $transaction->processedBy?->name,
                'created_at' => $transaction->transaction_date,
                'notes' => $transaction->notes,
                'type' => $transaction->type,
            ];
        });

        return response()->json([
            'data' => $payments,
        ]);
    }

    public function pay(Request $request, Bill $bill)
    {
        // Check if bill can be paid
        if ($bill->status !== 'pending') {
            return response()->json([
                'message' => 'Only pending bills can be paid.'
            ], 422);
        }

        $validated = $request->validate([
            'amount_paid' => ['required', 'numeric', 'min:0'],
            'cash_received' => ['required', 'numeric', 'min:0'],
            'change_amount' => ['required', 'numeric'],
            'payment_method' => ['required', 'string', 'in:cash'],
        ]);

        // Update bill status to paid
        $bill->status = 'paid';
        $bill->save();

        // Create transaction record
        BillTransaction::create([
            'bill_reference_number' => $bill->reference_number,
            'type' => 'payment',
            'amount' => $validated['amount_paid'],
            'cash_received' => $validated['cash_received'],
            'change' => $validated['change_amount'],
            'transaction_date' => now(),
            'payment_method' => $validated['payment_method'],
            'processed_by_user_id' => auth()->id(),
        ]);

        return response()->json([
            'data' => $bill,
        ]);
    }

    public function todayPayments(Request $request)
    {
        $today = now()->toDateString();

        $payments = Bill::where('status', 'paid')
            ->whereDate('updated_at', $today)
            ->get();

        return response()->json([
            'count' => $payments->count(),
            'total' => $payments->sum('total_due'),
        ]);
    }

    public function monthlyPayments(Request $request)
    {
        $currentMonth = now()->month;
        $currentYear = now()->year;

        $payments = Bill::where('status', 'paid')
            ->whereYear('updated_at', $currentYear)
            ->whereMonth('updated_at', $currentMonth)
            ->get();

        return response()->json([
            'count' => $payments->count(),
            'total' => $payments->sum('total_due'),
        ]);
    }

    public function monthlyCollectable(Request $request)
    {
        $currentMonth = now()->month;
        $currentYear = now()->year;

        $collectable = Bill::where('status', 'pending')
            ->whereYear('due_date', $currentYear)
            ->whereMonth('due_date', $currentMonth)
            ->get();

        return response()->json([
            'count' => $collectable->count(),
            'total' => $collectable->sum('total_due'),
        ]);
    }

    public function stats(Request $request)
    {
        $pending = Bill::where('status', 'pending')->count();

        $overdue = Bill::where('status', 'pending')
            ->where('due_date', '<', now()->toDateString())
            ->count();

        return response()->json([
            'pending' => $pending,
            'overdue' => $overdue,
        ]);
    }
    public function markPaid(Request $request, Bill $bill)
    {
        $validated = $request->validate([
            'status' => ['nullable', 'string', 'in:paid'],
        ]);

        // Apply penalty at payment time (if overdue beyond grace days).
        $settings = Setting::query()->orderBy('id')->first();
        $penaltyAfterDays = $settings ? (int) $settings->penalty_after_days : (int) env('PENALTY_AFTER_DAYS', 0);
        $penaltyAmount = $settings ? (float) $settings->penalty_amount : (float) env('PENALTY_AMOUNT', 0.00);

        $penalty = 0.0;
        if ($bill->due_date) {
            $threshold = Carbon::parse($bill->due_date)->addDays($penaltyAfterDays);
            if (now()->greaterThan($threshold) && $penaltyAmount > 0) {
                $penalty = $penaltyAmount;
            }
        }

        $bill->penalty = $penalty;
        $bill->total_due = (float) $bill->charges + (float) $bill->penalty;

        $bill->status = 'paid';
        $bill->save();

        // Create transaction record
        BillTransaction::create([
            'bill_reference_number' => $bill->reference_number,
            'type' => 'payment',
            'amount' => $bill->total_due,
            'cash_received' => $bill->total_due,
            'change' => 0,
            'transaction_date' => now(),
            'payment_method' => 'cash',
            'processed_by_user_id' => auth()->id(),
        ]);

        return response()->json([
            'data' => $bill,
        ]);
    }

    public function void(Request $request, Bill $bill)
    {
        // Check if bill can be voided
        if ($bill->status !== 'paid') {
            return response()->json([
                'message' => 'Only paid bills can be voided.'
            ], 422);
        }

        $validated = $request->validate([
            'reason' => ['nullable', 'string', 'max:1000'],
        ]);

        $bill->status = 'pending';
        $bill->void_reason = $validated['reason'] ?? null;
        $bill->save();

        // Create transaction record for void
        BillTransaction::create([
            'bill_reference_number' => $bill->reference_number,
            'type' => 'void',
            'amount' => $bill->total_due,
            'transaction_date' => now(),
            'processed_by_user_id' => auth()->id(),
            'notes' => $validated['reason'] ?? null,
        ]);

        return response()->json([
            'data' => $bill,
        ]);
    }

    public function transactions(Request $request)
    {
        $limit = $request->query('limit', 50);

        $transactions = BillTransaction::with(['bill.customer', 'processedBy'])
            ->orderBy('transaction_date', 'desc')
            ->limit($limit)
            ->get();

        return response()->json([
            'data' => $transactions,
        ]);
    }

    public function getPayment(Request $request, Bill $bill)
    {
        $transaction = BillTransaction::where('bill_reference_number', $bill->reference_number)
            ->where('type', 'payment')
            ->first();

        if (!$transaction) {
            return response()->json([
                'message' => 'No payment found for this bill.'
            ], 404);
        }

        return response()->json($transaction);
    }

    /**
     * Bulk upsert bills coming from the device.
     * Expects: { bills: [ {bill_id, reference_number, customer_id, reading_id, bill_date, rate_per_m3, charges, penalty, total_due, status, created_at, updated_at} ] }
     */
 public function sync(Request $request)
{
    $validated = $request->validate([
        'bills' => ['required', 'array', 'max:2000'],
        'bills.*.reference_number' => ['required', 'string', 'max:255'],
        'bills.*.customer_id' => ['nullable', 'integer'],
        'bills.*.reading_id' => ['nullable', 'integer'],
        'bills.*.device_uid' => ['nullable', 'string', 'max:255'],
        'bills.*.bill_date' => ['nullable', 'string'],
        'bills.*.rate_per_m3' => ['nullable', 'numeric'],
        'bills.*.charges' => ['nullable', 'numeric'],
        'bills.*.penalty' => ['nullable', 'numeric'],
        'bills.*.total_due' => ['nullable', 'numeric'],
        'bills.*.status' => ['nullable', 'string', 'max:255'],
        'bills.*.customer_account_number' => ['required', 'string', 'max:255'],
    ]);

    $bills = $validated['bills'];
    $processed = 0;
    $skipped = 0;

    foreach ($bills as $row) {
        // ONLY use account_no - NEVER trust customer_id from device!
        $customer = Customer::where('account_no', $row['customer_account_number'])->first();
        
        if (!$customer) {
            \Log::warning('Bill sync: Customer not found', [
                'account_no' => $row['customer_account_number']
            ]);
            $skipped++;
            continue;
        }

        // Find correct reading_id from OUR database
        $readingId = null;
        if (!empty($row['bill_date'])) {
            $billDate = Carbon::parse($row['bill_date']);
            
            // Try to find reading by customer_account_number and same month/year
            $reading = Reading::where('customer_account_number', $row['customer_account_number'])
                ->whereYear('reading_at', $billDate->year)
                ->whereMonth('reading_at', $billDate->month)
                ->orderBy('reading_at', 'desc')
                ->first();
            
            if ($reading) {
                $readingId = $reading->reading_id;
            }
        }

        // Skip if we can't find a reading_id
        if (!$readingId) {
            \Log::error('Bill sync: Cannot find reading_id', [
                'account' => $row['customer_account_number'],
                'bill_date' => $row['bill_date'] ?? 'null',
                'reference' => $row['reference_number']
            ]);
            $skipped++;
            continue;
        }

        Bill::updateOrCreate(
            ['reference_number' => $row['reference_number']],
            [
                'customer_id' => $customer->customer_id,
                'reading_id' => $readingId,
                'device_uid' => $row['device_uid'] ?? null,
                'bill_date' => $row['bill_date'] ?? null,
                'rate_per_m3' => $row['rate_per_m3'] ?? 0,
                'charges' => $row['charges'] ?? 0,
                'penalty' => $row['penalty'] ?? 0,
                'total_due' => $row['total_due'] ?? 0,
                'status' => $row['status'] ?? 'Pending',
                'customer_account_number' => $row['customer_account_number'],
            ]
        );

        $processed++;
    }

    \Log::info('Bills synced', ['processed' => $processed, 'skipped' => $skipped]);

    return response()->json([
        'processed' => $processed,
        'skipped' => $skipped,
    ]);
}
    /**
     * Bulk upsert bill transactions coming from the device.
     * Expects: { bill_transactions: [ {...} ] }
     */
    public function syncTransactions(Request $request)
    {
        \Log::info('Syncing bill transactions', ['count' => count($request->bill_transactions ?? [])]);

        $validated = $request->validate([
            // Allow large device exports; client will still batch to keep requests small.
            'bill_transactions' => ['required', 'array', 'max:2000'],
            'bill_transactions.*.bill_transaction_id' => ['nullable', 'integer'],
            'bill_transactions.*.bill_id' => ['nullable', 'integer'],
            'bill_transactions.*.bill_reference_number' => ['required', 'string'],
            'bill_transactions.*.type' => ['required', 'string'],
            'bill_transactions.*.source' => ['required', 'string'],
            'bill_transactions.*.amount' => ['required', 'numeric'],
            'bill_transactions.*.cash_received' => ['nullable', 'numeric'],
            'bill_transactions.*.change' => ['required', 'numeric'],
            'bill_transactions.*.transaction_date' => ['required', 'string'],
            'bill_transactions.*.payment_method' => ['required', 'string'],
            'bill_transactions.*.processed_by_device_uid' => ['nullable', 'string'],
            'bill_transactions.*.notes' => ['nullable', 'string'],
            'bill_transactions.*.created_at' => ['nullable', 'string'],
            'bill_transactions.*.updated_at' => ['nullable', 'string'],
        ]);

        $transactions = $validated['bill_transactions'];
        $processed = 0;

        foreach ($transactions as $row) {
            try {
                BillTransaction::updateOrCreate(
                    [
                        'bill_reference_number' => $row['bill_reference_number'],
                        'transaction_date' => $row['transaction_date'],
                        'amount' => $row['amount'],
                    ],
                    [
                        'device_uid' => $row['processed_by_device_uid'] ?? null,
                        'type' => $row['type'],
                        'source' => $row['source'],
                        'cash_received' => $row['cash_received'] ?? null,
                        'change' => $row['change'],
                        'payment_method' => $row['payment_method'],
                        'processed_by_device_uid' => $row['processed_by_device_uid'] ?? null,
                        'notes' => $row['notes'] ?? null,
                        'Synced' => true,
                        'last_sync' => now(),
                        'created_at' => $row['created_at'] ?? now(),
                        'updated_at' => $row['updated_at'] ?? now(),
                    ]
                );

                $processed++;
            } catch (\Exception $e) {
                \Log::error('Failed to sync bill transaction', [
                    'bill_reference_number' => $row['bill_reference_number'],
                    'transaction_date' => $row['transaction_date'],
                    'amount' => $row['amount'],
                    'error' => $e->getMessage()
                ]);
            }
        }

        \Log::info('Bill transactions synced', ['processed' => $processed]);

        return response()->json([
            'processed' => $processed,
        ]);
    }

    /**
     * Mark bill transactions as synced after successful device sync.
     */
    public function markTransactionsSynced(Request $request)
    {
        $validated = $request->validate([
            'bill_transaction_ids' => ['required', 'array'],
            'bill_transaction_ids.*' => ['required', 'integer'],
        ]);

        $updated = BillTransaction::whereIn('bill_transaction_id', $validated['bill_transaction_ids'])
            ->update([
                'Synced' => true,
                'last_sync' => now(),
            ]);

        return response()->json([
            'updated' => $updated,
            'message' => "{$updated} bill transactions marked as synced",
        ]);
    }

    /**
     * Mark bills as synced after successful device sync.
     */
    public function markSynced(Request $request)
    {
        $validated = $request->validate([
            'bill_ids' => ['required', 'array'],
            'bill_ids.*' => ['required', 'integer'],
        ]);

        $updated = Bill::whereIn('bill_id', $validated['bill_ids'])
            ->update([
                'Synced' => true,
                'last_sync' => now(),
            ]);

        return response()->json([
            'updated' => $updated,
            'message' => "{$updated} bills marked as synced",
        ]);
    }
}
